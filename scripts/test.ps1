param([string]$Compiler = "g++")
$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
& (Join-Path $PSScriptRoot "build.ps1") -Compiler $Compiler
$executable = Join-Path (Join-Path $projectRoot "build") "scanner_tests.exe"
$sources = @(
    (Join-Path $projectRoot "tests/scanner_tests.cpp"),
    (Join-Path $projectRoot "src/scanner.cpp"),
    (Join-Path $projectRoot "src/application.cpp"),
    (Join-Path $projectRoot "src/file_index.cpp")
)
& $Compiler -std=c++17 -Wall -Wextra -Wpedantic @sources -o $executable
if ($LASTEXITCODE -ne 0) { throw "Test build failed. An old test executable was not run." }
& $executable
if ($LASTEXITCODE -ne 0) { throw "Scanner tests failed. Read FAIL lines above." }

$indexExecutable = Join-Path (Join-Path $projectRoot "build") "file_index_tests.exe"
& $Compiler -std=c++17 -Wall -Wextra -Wpedantic (Join-Path $projectRoot "tests/file_index_tests.cpp") (Join-Path $projectRoot "src/file_index.cpp") -o $indexExecutable
if ($LASTEXITCODE -ne 0) { throw "Index test build failed." }
& $indexExecutable
if ($LASTEXITCODE -ne 0) { throw "Index tests failed." }
