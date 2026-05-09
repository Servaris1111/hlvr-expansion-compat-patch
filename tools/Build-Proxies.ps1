[CmdletBinding()]
param(
    [string]$LlvmMingwBin = $env:LLVM_MINGW_BIN,
    [string]$HLSDKDir = $env:HLSDK_DIR
)

$ErrorActionPreference = "Stop"
$Root = Resolve-Path (Join-Path $PSScriptRoot "..")

if (-not $LlvmMingwBin) {
    throw "Pass -LlvmMingwBin or set LLVM_MINGW_BIN to the llvm-mingw bin folder."
}
if (-not $HLSDKDir) {
    throw "Pass -HLSDKDir or set HLSDK_DIR to an SDK/source tree containing dlls/extdll.h and engine/eiface.h."
}

$cxx = Join-Path $LlvmMingwBin "i686-w64-mingw32-g++.exe"
if (-not (Test-Path $cxx)) {
    throw "Compiler not found: $cxx"
}

$includes = @(
    "-I$HLSDKDir\dlls",
    "-I$HLSDKDir\engine",
    "-I$HLSDKDir\common",
    "-I$HLSDKDir\pm_shared",
    "-I$HLSDKDir\game_shared",
    "-I$HLSDKDir\public"
)

function Build-Proxy {
    param(
        [string]$Out,
        [string]$Def,
        [string]$Source
    )

    $args = @(
        "-std=c++17",
        "-O2",
        "-DNDEBUG",
        "-DWIN32",
        "-D_WINDOWS"
    ) + $includes + @(
        "-shared",
        "-static-libgcc",
        "-static-libstdc++",
        "-Wl,--kill-at",
        "-o",
        $Out,
        $Source,
        $Def
    )

    & $cxx @args
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for $Out"
    }
}

Build-Proxy -Out (Join-Path $Root "bin\opfor\opfor.dll") -Def (Join-Path $Root "src\goldsrc-proxy\opfor_proxy.def") -Source (Join-Path $Root "src\goldsrc-proxy\goldsrc_proxy.cpp")
Build-Proxy -Out (Join-Path $Root "bin\bshift\hl.dll") -Def (Join-Path $Root "src\goldsrc-proxy\bshift_hl_proxy.def") -Source (Join-Path $Root "src\goldsrc-proxy\bshift_proxy.cpp")

Get-FileHash -Algorithm SHA256 -Path (Join-Path $Root "bin\opfor\opfor.dll"), (Join-Path $Root "bin\bshift\hl.dll")
