param([string]$Compiler = "g++")
$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildDirectory = Join-Path $projectRoot "build"
New-Item -ItemType Directory -Path $buildDirectory -Force | Out-Null
$sources = @("main.cpp", "cli_dispatcher.cpp", "menu.cpp", "application.cpp", "scanner.cpp", "file_index.cpp", "rule_config.cpp", "storage_analyzer.cpp", "duplicate_detector.cpp") | ForEach-Object {
Join-Path (Join-Path $projectRoot "src") $_
}
$executable = Join-Path $buildDirectory "folderflow.exe"
& $Compiler -std=c++17 -Wall -Wextra -Wpedantic @sources -o $executable
if ($LASTEXITCODE -ne 0) { throw "Build failed. Do not run an older executable." }
Write-Host "BUILD SUCCESSFUL: $executable" -ForegroundColor Green
