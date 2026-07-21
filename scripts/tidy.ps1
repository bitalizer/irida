# SPDX-License-Identifier: BUSL-1.1
# Runs clang-tidy over the irida-core / irida-api sources using the compile
# database from a configured build directory. Third-party and system headers
# are filtered out by .clang-tidy's HeaderFilterRegex.
#
#   . .\scripts\devenv.ps1        # once per shell, loads MSVC + clang-tidy
#   .\scripts\tidy.ps1            # lint (report only)
#   .\scripts\tidy.ps1 -Fix       # apply clang-tidy's auto-fixes in place
#
# Pass -BuildDir to point at a preset other than the default headless build.
param(
    [string]$BuildDir = "build/headless-debug",
    [switch]$Fix
)

$ErrorActionPreference = "Stop"

$tidy = (Get-Command clang-tidy -ErrorAction SilentlyContinue)?.Source
if (-not $tidy) {
    $tidy = "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-tidy.exe"
}
if (-not (Test-Path $tidy)) {
    throw "clang-tidy not found. Load the dev environment first (. .\scripts\devenv.ps1)."
}

if (-not (Test-Path "$BuildDir/compile_commands.json")) {
    throw "No compile_commands.json in $BuildDir. Configure it first: cmake --preset headless-debug"
}

# Only lint our own translation units, never third-party adapters' vendored code.
$sources = Get-ChildItem -Recurse -Include *.cpp -Path irida-core, irida-api |
    Where-Object { $_.FullName -notmatch "\\tests\\" }

$tidyArgs = @("-p", $BuildDir)
if ($Fix) { $tidyArgs += "--fix" }

$failed = $false
foreach ($src in $sources) {
    Write-Host "tidy: $($src.FullName)"
    & $tidy @tidyArgs $src.FullName
    if ($LASTEXITCODE -ne 0) { $failed = $true }
}

if ($failed) { exit 1 }
