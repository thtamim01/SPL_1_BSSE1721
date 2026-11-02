$ErrorActionPreference = "Stop"
$fixtureRoot = Join-Path $env:TEMP ("FolderFlow-demo-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $fixtureRoot | Out-Null
New-Item -ItemType Directory -Path (Join-Path $fixtureRoot "nested") | Out-Null
New-Item -ItemType Directory -Path (Join-Path $fixtureRoot "empty") | Out-Null
New-Item -ItemType Directory -Path (Join-Path $fixtureRoot ".git") | Out-Null
[System.IO.File]::WriteAllBytes((Join-Path $fixtureRoot "first.bin"), [byte[]]::new(500000))
[System.IO.File]::WriteAllBytes((Join-Path $fixtureRoot "nested/second.bin"), [byte[]]::new(1000000))
[System.IO.File]::WriteAllBytes((Join-Path $fixtureRoot ".git/ignored.bin"), [byte[]]::new(1000000))
Write-Output $fixtureRoot
