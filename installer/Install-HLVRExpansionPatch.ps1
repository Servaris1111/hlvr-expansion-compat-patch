[CmdletBinding()]
param(
    [string]$HalfLifePath,
    [string]$HLVRPath,
    [switch]$SkipHLFixes,
    [switch]$ForceHLFixes,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

$PatchRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$BackupStamp = Get-Date -Format "yyyyMMdd-HHmmss"

function Write-Info {
    param([string]$Message)
    Write-Host "[HLVR Expansion Patch] $Message"
}

function Invoke-PatchAction {
    param(
        [string]$Description,
        [scriptblock]$Action
    )

    if ($DryRun) {
        Write-Info "DRY RUN: $Description"
        return
    }

    Write-Info $Description
    & $Action
}

function Get-SteamRoot {
    $candidates = @()

    try {
        $steamReg = Get-ItemProperty -Path "HKCU:\Software\Valve\Steam" -ErrorAction Stop
        if ($steamReg.SteamPath) {
            $candidates += ($steamReg.SteamPath -replace "/", "\")
        }
    } catch {}

    $candidates += @(
        "${env:ProgramFiles(x86)}\Steam",
        "$env:ProgramFiles\Steam"
    )

    foreach ($candidate in $candidates | Select-Object -Unique) {
        if ($candidate -and (Test-Path (Join-Path $candidate "steam.exe"))) {
            return (Resolve-Path $candidate).Path
        }
    }

    return $null
}

function Get-SteamLibraryPaths {
    $steamRoot = Get-SteamRoot
    $libraries = @()

    if ($steamRoot) {
        $libraries += $steamRoot
        $libraryFile = Join-Path $steamRoot "steamapps\libraryfolders.vdf"
        if (Test-Path $libraryFile) {
            foreach ($line in Get-Content $libraryFile) {
                if ($line -match '"path"\s+"([^"]+)"') {
                    $libraries += ($Matches[1] -replace "\\\\", "\")
                }
            }
        }
    }

    $libraries += @(
        "C:\Program Files (x86)\Steam",
        "C:\Program Files\Steam",
        "D:\SteamLibrary",
        "E:\SteamLibrary",
        "F:\SteamLibrary"
    )

    return $libraries | Where-Object { $_ -and (Test-Path $_) } | Select-Object -Unique
}

function Find-SteamCommonPath {
    param([string]$FolderName)

    foreach ($library in Get-SteamLibraryPaths) {
        $candidate = Join-Path $library "steamapps\common\$FolderName"
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    return $null
}

function Assert-Directory {
    param(
        [string]$Path,
        [string]$Name
    )

    if (-not $Path -or -not (Test-Path $Path)) {
        throw "$Name was not found. Pass -$Name `"C:\Path\To\Folder`" manually."
    }
}

function Assert-File {
    param(
        [string]$Path,
        [string]$Description
    )

    if (-not (Test-Path $Path)) {
        throw "Missing $Description at $Path"
    }
}

function Backup-File {
    param(
        [string]$Path,
        [string]$Root
    )

    if (-not (Test-Path $Path)) {
        return
    }

    $relative = Resolve-Path $Path | ForEach-Object {
        $_.Path.Substring((Resolve-Path $Root).Path.Length).TrimStart("\")
    }
    $backupRoot = Join-Path $Root "_hlvr_expansion_patch_backup\$BackupStamp"
    $destination = Join-Path $backupRoot $relative
    $destinationDir = Split-Path $destination -Parent

    New-Item -ItemType Directory -Force -Path $destinationDir | Out-Null
    Copy-Item -Force -Path $Path -Destination $destination
}

function Copy-DirectoryContents {
    param(
        [string]$Source,
        [string]$Destination
    )

    if (-not (Test-Path $Source)) {
        Write-Info "Skipping missing optional folder: $Source"
        return
    }

    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    Copy-Item -Force -Recurse -Path (Join-Path $Source "*") -Destination $Destination
}

function Set-ConfigValue {
    param(
        [string]$Path,
        [string]$Name,
        [string]$Value
    )

    $line = "$Name `"$Value`""
    if (Test-Path $Path) {
        $content = Get-Content -Path $Path
    } else {
        $content = @()
    }

    $found = $false
    $updated = foreach ($existingLine in $content) {
        if ($existingLine -match "^\s*$([regex]::Escape($Name))\s+") {
            $found = $true
            $line
        } else {
            $existingLine
        }
    }

    if (-not $found) {
        $updated += $line
    }

    Set-Content -Path $Path -Value $updated -Encoding ASCII
}

function Set-MarkedBlock {
    param(
        [string]$Path,
        [string[]]$Lines
    )

    $start = "// BEGIN HLVR EXPANSION COMPAT PATCH"
    $end = "// END HLVR EXPANSION COMPAT PATCH"
    $block = @($start) + $Lines + @($end)

    if (Test-Path $Path) {
        $raw = Get-Content -Path $Path -Raw
    } else {
        $raw = ""
    }

    $pattern = "(?s)\r?\n?// BEGIN HLVR EXPANSION COMPAT PATCH.*?// END HLVR EXPANSION COMPAT PATCH\r?\n?"
    $raw = [regex]::Replace($raw, $pattern, "")
    $raw = $raw.TrimEnd()
    if ($raw.Length -gt 0) {
        $raw += "`r`n"
    }
    $raw += ($block -join "`r`n") + "`r`n"

    Set-Content -Path $Path -Value $raw -Encoding ASCII
}

function Set-LiblistGameDll {
    param(
        [string]$Path,
        [string]$GameDll
    )

    Assert-File $Path "liblist.gam"

    $lines = Get-Content -Path $Path
    $found = $false
    $updated = foreach ($line in $lines) {
        if ($line -match "^\s*gamedll\s+") {
            $found = $true
            "gamedll `"$GameDll`""
        } else {
            $line
        }
    }

    if (-not $found) {
        $updated += "gamedll `"$GameDll`""
    }

    Set-Content -Path $Path -Value $updated -Encoding ASCII
}

function Install-ProxyDll {
    param(
        [string]$ModDir,
        [string]$DllName,
        [string]$StockName,
        [string]$PackageDll
    )

    $dllDir = Join-Path $ModDir "dlls"
    $target = Join-Path $dllDir $DllName
    $stock = Join-Path $dllDir $StockName

    Assert-File $target "$DllName stock game DLL"
    Assert-File $PackageDll "packaged proxy DLL"

    $packageHash = (Get-FileHash -Algorithm SHA256 -Path $PackageDll).Hash
    $targetHash = (Get-FileHash -Algorithm SHA256 -Path $target).Hash

    if (-not (Test-Path $stock)) {
        if ($targetHash -eq $packageHash) {
            throw "$target is already the proxy but $stock is missing. Restore the original game DLL first."
        }
        Backup-File -Path $target -Root $HLVRPath
        Copy-Item -Force -Path $target -Destination $stock
    }

    Backup-File -Path $target -Root $HLVRPath
    Copy-Item -Force -Path $PackageDll -Destination $target
}

function Install-Launcher {
    param(
        [string]$Name,
        [string]$GameDir,
        [string]$StartMap
    )

    $path = Join-Path $HLVRPath $Name
    $content = @(
        "@echo off",
        "setlocal",
        "set `"HLVR_DIR=%~dp0`"",
        "set `"SteamAppId=1908720`"",
        "set `"SteamGameId=1908720`"",
        "set `"SteamOverlayGameId=1908720`"",
        "",
        "copy /Y `"%HLVR_DIR%valve\cl_dlls\client.dll`" `"%HLVR_DIR%$GameDir\cl_dlls\client.dll`" >nul",
        "xcopy `"%HLVR_DIR%valve\actions`" `"%HLVR_DIR%$GameDir\actions\`" /E /I /Y >nul",
        "",
        "pushd `"%HLVR_DIR%`"",
        "start `"`" `"%HLVR_DIR%hl.exe`" -steam -game $GameDir -console -condebug +exec autoexec.cfg +vr_use_fmod 0 +map $StartMap",
        "popd",
        "endlocal"
    )

    Set-Content -Path $path -Value $content -Encoding ASCII
}

function Install-ModCommon {
    param(
        [string]$GameDir,
        [string]$SourceDir
    )

    $destination = Join-Path $HLVRPath $GameDir
    Copy-DirectoryContents -Source $SourceDir -Destination $destination

    $clientSource = Join-Path $HLVRPath "valve\cl_dlls\client.dll"
    $clientDestination = Join-Path $destination "cl_dlls\client.dll"
    Assert-File $clientSource "HLVR client.dll"
    New-Item -ItemType Directory -Force -Path (Split-Path $clientDestination -Parent) | Out-Null
    Backup-File -Path $clientDestination -Root $HLVRPath
    Copy-Item -Force -Path $clientSource -Destination $clientDestination

    Copy-DirectoryContents -Source (Join-Path $HLVRPath "valve\actions") -Destination (Join-Path $destination "actions")

    Set-MarkedBlock -Path (Join-Path $destination "autoexec.cfg") -Lines @(
        'alias VModEnable ""',
        'alias vrupd_hmd ""',
        'alias vrupdctrl ""',
        'alias vr_flashlight ""',
        'alias vr_teleporter ""',
        'alias vr_anlgfire ""',
        'alias vr_lngjump ""',
        'alias vr_restartmap ""',
        'alias vr_wpnanim ""',
        'alias vr_muzzleflash ""',
        'alias vrtele ""',
        'alias vrspeech ""',
        'volume "0.8"',
        'MP3Volume "0.8"',
        'bgmvolume "1"',
        'hisound "1"',
        'suitvolume "0.25"',
        'vr_use_fmod "0"'
    )

    Set-ConfigValue -Path (Join-Path $destination "config.cfg") -Name "vr_use_fmod" -Value "0"
}

function Install-HLFixes {
    if ($SkipHLFixes) {
        Write-Info "Skipping HLFixes by request."
        return
    }

    $required = @("HLFixes.dll", "hl.fix", "sw.fix")
    $alreadyInstalled = $true
    foreach ($file in $required) {
        if (-not (Test-Path (Join-Path $HLVRPath $file))) {
            $alreadyInstalled = $false
        }
    }

    if ($alreadyInstalled -and -not $ForceHLFixes) {
        Write-Info "HLFixes appears to already be installed in the HLVR folder."
        return
    }

    if ($DryRun) {
        Write-Info "DRY RUN: Would download and run the official HLFixes Windows installer."
        return
    }

    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) "hlvr-expansion-hlfixes-$BackupStamp"
    New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null
    $zipPath = Join-Path $tempRoot "HLFixes-Windows.zip"

    Write-Info "Downloading the latest official HLFixes Windows release..."
    $release = Invoke-RestMethod -Uri "https://api.github.com/repos/IntriguingTiles/HLFixes/releases/latest"
    $asset = $release.assets | Where-Object { $_.name -eq "Windows.zip" } | Select-Object -First 1
    if (-not $asset) {
        throw "Could not find Windows.zip in the latest HLFixes release."
    }

    Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $zipPath
    Expand-Archive -Force -Path $zipPath -DestinationPath $tempRoot

    $installer = Get-ChildItem -Path $tempRoot -Recurse -Filter "Installer.exe" | Select-Object -First 1
    if (-not $installer) {
        throw "HLFixes Installer.exe was not found after extracting Windows.zip."
    }

    Write-Info "Launching the official HLFixes installer. Choose the HLVR folder if prompted: $HLVRPath"
    Start-Process -FilePath $installer.FullName -WorkingDirectory $installer.DirectoryName -Wait
}

function Assert-HalfLifeNotRunning {
    $running = Get-Process -Name "hl" -ErrorAction SilentlyContinue
    if ($running) {
        throw "Half-Life/HLVR is running. Close it before installing the patch."
    }
}

if (-not $HalfLifePath) {
    $HalfLifePath = Find-SteamCommonPath "Half-Life"
}
if (-not $HLVRPath) {
    $HLVRPath = Find-SteamCommonPath "Half-Life VR Mod"
}

Assert-Directory -Path $HalfLifePath -Name "HalfLifePath"
Assert-Directory -Path $HLVRPath -Name "HLVRPath"
Assert-File -Path (Join-Path $HalfLifePath "hl.exe") -Description "Half-Life hl.exe"
Assert-File -Path (Join-Path $HLVRPath "hl.exe") -Description "Half-Life VR Mod hl.exe"
if (-not $DryRun) {
    Assert-HalfLifeNotRunning
}

$HalfLifePath = (Resolve-Path $HalfLifePath).Path
$HLVRPath = (Resolve-Path $HLVRPath).Path

Write-Info "Half-Life: $HalfLifePath"
Write-Info "Half-Life VR Mod: $HLVRPath"

$opforSource = Join-Path $HalfLifePath "gearbox"
$bshiftSource = Join-Path $HalfLifePath "bshift"
$opforPackageDll = Join-Path $PatchRoot "bin\opfor\opfor.dll"
$bshiftPackageDll = Join-Path $PatchRoot "bin\bshift\hl.dll"

Invoke-PatchAction "Installing Opposing Force files and HLVR client integration" {
    Install-ModCommon -GameDir "gearbox" -SourceDir $opforSource
    Copy-DirectoryContents -Source (Join-Path $HalfLifePath "gearbox_hd") -Destination (Join-Path $HLVRPath "gearbox_hd")
    Install-ProxyDll -ModDir (Join-Path $HLVRPath "gearbox") -DllName "opfor.dll" -StockName "opfor_stock.dll" -PackageDll $opforPackageDll
    Set-LiblistGameDll -Path (Join-Path $HLVRPath "gearbox\liblist.gam") -GameDll "dlls\opfor.dll"
    Set-ConfigValue -Path (Join-Path $HLVRPath "gearbox\skillopfor.cfg") -Name "sk_plr_hornet_dmg1" -Value "10"
    Set-ConfigValue -Path (Join-Path $HLVRPath "gearbox\skillopfor.cfg") -Name "sk_plr_hornet_dmg2" -Value "10"
    Set-ConfigValue -Path (Join-Path $HLVRPath "gearbox\skillopfor.cfg") -Name "sk_plr_hornet_dmg3" -Value "10"
    Install-Launcher -Name "Launch Opposing Force VR.bat" -GameDir "gearbox" -StartMap "of0a0"
}

Invoke-PatchAction "Installing Blue Shift files and HLVR client integration" {
    Install-ModCommon -GameDir "bshift" -SourceDir $bshiftSource
    Copy-DirectoryContents -Source (Join-Path $HalfLifePath "bshift_hd") -Destination (Join-Path $HLVRPath "bshift_hd")
    Install-ProxyDll -ModDir (Join-Path $HLVRPath "bshift") -DllName "hl.dll" -StockName "hl_stock.dll" -PackageDll $bshiftPackageDll
    Set-LiblistGameDll -Path (Join-Path $HLVRPath "bshift\liblist.gam") -GameDll "dlls\hl.dll"
    Install-Launcher -Name "Launch Blue Shift VR.bat" -GameDir "bshift" -StartMap "ba_tram1"
}

Install-HLFixes

Write-Info "Done. Launchers are in: $HLVRPath"
Write-Info "Use Launch Opposing Force VR.bat or Launch Blue Shift VR.bat."
Write-Info "Do not use Steam's Change Game menu for these VR expansion launches."
Write-Info "Opposing Force-only weapons are mapped to HLVR-supported base Half-Life weapon paths where possible."
