[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("Disable", "Restore")]
    [string]$Mode,

    [Parameter(Mandatory = $true)]
    [string]$Path,

    [Parameter(Mandatory = $true)]
    [string]$BackupPath
)

$ErrorActionPreference = "Stop"

function Disable-Fmod {
    $lines = New-Object System.Collections.Generic.List[string]
    $found = $false

    if (Test-Path -LiteralPath $Path) {
        $backupDir = Split-Path -Path $BackupPath -Parent
        if ($backupDir) {
            New-Item -ItemType Directory -Force -Path $backupDir | Out-Null
        }

        if (-not (Test-Path -LiteralPath $BackupPath)) {
            Copy-Item -LiteralPath $Path -Destination $BackupPath -Force
        }

        foreach ($line in Get-Content -LiteralPath $Path) {
            if ($line -match '^\s*vr_use_fmod\s*=') {
                if (-not $found) {
                    $lines.Add("vr_use_fmod=0")
                    $found = $true
                }
            } else {
                $lines.Add($line)
            }
        }
    }

    if (-not $found) {
        $lines.Add("vr_use_fmod=0")
    }

    Set-Content -LiteralPath $Path -Value $lines -Encoding ASCII
}

function Restore-FmodConfig {
    if (Test-Path -LiteralPath $BackupPath) {
        Copy-Item -LiteralPath $BackupPath -Destination $Path -Force
        Remove-Item -LiteralPath $BackupPath -Force
    }
}

switch ($Mode) {
    "Disable" { Disable-Fmod }
    "Restore" { Restore-FmodConfig }
}
