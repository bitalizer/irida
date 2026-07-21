# SPDX-License-Identifier: BUSL-1.1
# Copyright (c) 2026 Bitalizer.
#
# Load the MSVC x64 build environment into the CURRENT PowerShell session, once.
#
# Why: the MSVC toolchain (cl.exe + INCLUDE/LIB) is not on PATH by default, so a
# bare shell can't compile (it fails to find even <stddef.h>). The usual fix is to
# run vcvars64.bat, but that re-initializes the whole environment on every command
# (~10s each) and must be re-run because PowerShell tool calls don't share state.
#
# Instead: dot-source this ONCE at the start of a shell, then run plain cmake/ninja:
#
#     . .\scripts\devenv.ps1
#     cmake -G Ninja -DIRIDA_GUI=OFF -B build\headless-debug
#     cmake --build build\headless-debug
#     ctest --test-dir build\headless-debug
#
# It captures the variables vcvars64.bat sets (PATH, INCLUDE, LIB, LIBPATH, ...)
# and imports them into $env: for the rest of the session. Idempotent: running it
# again is harmless. Discovers the VS install via vswhere (no hard-coded version).

$ErrorActionPreference = 'Stop'

if ($env:IRIDA_DEVENV_LOADED -eq '1') {
    Write-Host "devenv: MSVC environment already loaded."
    return
}

# Locate the VS installation (any edition) without hard-coding a version.
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "devenv: vswhere.exe not found at $vswhere — is Visual Studio installed?"
}
$vsPath = & $vswhere -latest -products * -property installationPath
if (-not $vsPath) {
    throw "devenv: no Visual Studio installation found via vswhere."
}
$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) {
    throw "devenv: vcvars64.bat not found at $vcvars."
}

# Run vcvars in a child cmd and capture the resulting environment, then import it.
# `set` prints every VAR=VALUE after vcvars has configured the shell.
cmd /c "`"$vcvars`" >nul 2>&1 && set" | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') {
        Set-Item -Path "Env:$($matches[1])" -Value $matches[2]
    }
}

$env:IRIDA_DEVENV_LOADED = '1'
Write-Host "devenv: MSVC x64 environment loaded (cl.exe, INCLUDE, LIB ready)."

# QT_ROOT feeds CMAKE_PREFIX_PATH in the GUI presets, keeping the machine-specific
# Qt path out of the tracked CMakePresets.json. Honor an existing value; otherwise
# discover the newest C:\Qt\6.*\msvc*_64 kit.
if (-not $env:QT_ROOT) {
    $qtKit = Get-ChildItem -Path 'C:\Qt' -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^6\.' } |
        Sort-Object Name -Descending |
        ForEach-Object { Get-ChildItem -Path $_.FullName -Directory -Filter 'msvc*_64' -ErrorAction SilentlyContinue } |
        Select-Object -First 1
    if ($qtKit) {
        $env:QT_ROOT = $qtKit.FullName
    }
}
if ($env:QT_ROOT) {
    Write-Host "devenv: QT_ROOT = $env:QT_ROOT"
} else {
    Write-Host "devenv: QT_ROOT not set and no C:\Qt\6.* kit found (GUI presets need it)."
}
