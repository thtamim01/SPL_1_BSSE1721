param([string]$Compiler = "g++")
$ErrorActionPreference = "Stop"
$projectRoot = Split-Path -Parent $PSScriptRoot
& (Join-Path $PSScriptRoot "build.ps1") -Compiler $Compiler
$executable = Join-Path (Join-Path $projectRoot "build") "scanner_tests.exe"
$sources = @(
    (Join-Path $projectRoot "tests/scanner_tests.cpp"),
    (Join-Path $projectRoot "src/scanner.cpp"),
    (Join-Path $projectRoot "src/application.cpp"),
    (Join-Path $projectRoot "src/file_index.cpp"),
    (Join-Path $projectRoot "src/storage_analyzer.cpp")
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

$storageExecutable = Join-Path (Join-Path $projectRoot "build") "storage_analyzer_tests.exe"
$storageSources = @(
    (Join-Path $projectRoot "tests/storage_analyzer_tests.cpp"),
    (Join-Path $projectRoot "src/storage_analyzer.cpp")
)
& $Compiler -std=c++17 -Wall -Wextra -Wpedantic @storageSources -o $storageExecutable
if ($LASTEXITCODE -ne 0) { throw "Storage analyzer test build failed." }
& $storageExecutable
if ($LASTEXITCODE -ne 0) { throw "Storage analyzer tests failed." }

$menuExecutable = Join-Path (Join-Path $projectRoot "build") "menu_tests.exe"
$menuSources = @(
    (Join-Path $projectRoot "tests/menu_tests.cpp"),
    (Join-Path $projectRoot "src/menu.cpp")
)
& $Compiler -std=c++17 -Wall -Wextra -Wpedantic @menuSources -o $menuExecutable
if ($LASTEXITCODE -ne 0) { throw "Menu test build failed." }
Push-Location $projectRoot
try {
    & $menuExecutable
    if ($LASTEXITCODE -ne 0) { throw "Menu tests failed." }
} finally {
    Pop-Location
}

$cliExecutable = Join-Path (Join-Path $projectRoot "build") "cli_dispatcher_tests.exe"
$cliSources = @("tests/cli_dispatcher_tests.cpp", "src/cli_dispatcher.cpp", "src/menu.cpp",
                "src/application.cpp", "src/scanner.cpp", "src/file_index.cpp",
                "src/rule_config.cpp", "src/storage_analyzer.cpp") | ForEach-Object {
    Join-Path $projectRoot $_
}
& $Compiler -std=c++17 -Wall -Wextra -Wpedantic @cliSources -o $cliExecutable
if ($LASTEXITCODE -ne 0) { throw "CLI dispatcher test build failed." }
& $cliExecutable
if ($LASTEXITCODE -ne 0) { throw "CLI dispatcher tests failed." }
