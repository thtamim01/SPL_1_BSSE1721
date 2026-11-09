# FolderFlow Guardian

Automated File Organization, Storage Analysis, Versioning and Recovery System.
SPL-1, IIT, University of Dhaka.
Student: Md. Tasnimul Hasan Tamim (BSSE1721).
C++17; primary platform: Windows, VS Code, MSYS2 GCC/G++.

## Current checkpoint

The scanner accepts a root, traverses ordinary child folders, returns relative
paths/type/extension/size/modification metadata, and prints file/folder counts
and logical byte/decimal KB/MB totals. .git is excluded. Links and non-regular
entries are skipped. Inaccessible content or measurement failures produce
issues and partial results instead of silently complete totals.

Saved indexing, filename search and index comparison are implemented. Phase 3
adds extension rules, evaluation, dry-run preview, and Windows organization
and undo. Phase 4.1 adds a read-only storage report with counts and bytes by
extension and rankings for files and folders.

    .\build\folderflow.exe --analyze-storage "C:\Path\To\Folder" 5

See [Storage analysis](learnings/phase4-4.1-storage-analysis.md) for the demo,
algorithm and references. Duplicate detection, versioning and snapshots are
future work.

## Build and test on Windows

Open PowerShell at the repository root with g++ on PATH:

    .\scripts\build.ps1
    .\scripts\test.ps1

If local script policy blocks execution, use a process-scoped invocation:

    powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\test.ps1

No global policy change is required. The test script builds the application,
then the tests, and stops after compilation/test failures. Read PASS/FAIL/SKIP
separately; unavailable privileges can make link tests SKIP.

Find verified, content-identical files without changing them:

    .\build\folderflow.exe --find-duplicates "C:\path\to\folder"

The interactive menu also offers option 7. See the
[duplicate detection guide](learnings/phase4-4.4-duplicate-detection.md)
for the algorithm, limitations, and a disposable Windows demonstration.

Run the interactive menu from the repository root:

    .\build\folderflow.exe

Choose a number for scanning, storage analysis, saved index search, or an
organization preview. Windows also offers organizing and undoing with
confirmation. See the [console menu guide](learnings/phase4-4.2-interactive-menu.md).

For the original one-folder scanner prompt, run `folderflow.exe --scan`.
Press Enter at that prompt to scan the current directory, or supply the folder
directly with `folderflow.exe --scan "C:\Path With Spaces"`. Scanner exit codes
are 0 for complete, 1 for invalid or missing input, and 2 for incomplete.
Relative output paths use forward slashes.

## Disposable demonstration

    $fixture = & .\demo\create_fixture.ps1
    $fixture | .\build\folderflow.exe --scan

Expected: 2 files, 2 folders, 1500000 bytes, 1500.00 KB, 1.50 MB.
The fixture is uniquely created under TEMP; no personal files are modified.

## Verification boundaries

Fresh Linux GCC 13.3.0 builds passed with -Werror. The runner reported 25 passed,
0 failed, 0 skipped. The real executable passed the known fixture and invalid
root. See docs/evidence/phase1-1.5-linux.txt.
New PowerShell scripts and Windows-specific code have not run in this Linux
workspace. Current Windows execution remains required.

Windows conservatively skips all reparse points, including cloud placeholders.
The read-only scanner is not an atomic snapshot or protection against hostile
concurrent path replacement. Sizes are per-path logical lengths, not physical
allocation; hard links may count data multiple times. File times are filesystem
clock values, not yet a portable persistence format. No benchmark is claimed.

## Navigation

- [Master phase tracker](docs/DEVELOPMENT_ROADMAP.md)
- [Dated progress and commit evidence](docs/PROGRESS_LOG.md)
- [Four-session board and midterm checklist](docs/MIDTERM_READINESS.md)
- [Architecture](docs/architecture.md)
- [Learning manuals](learnings/README.md)
- [Test instructions](tests/README.md)
- [Historical manual tests](docs/testing.md)
- [Viva notes](docs/viva-notes.md)

New branches use phase/subphase IDs. Historical branches/commits remain intact.
Tested locally, published on a branch, and integrated into main are distinct
checkpoints.
## Saved index demo

After building FolderFlow Guardian, run these commands in PowerShell from the project folder.

```powershell
$demo = New-Item -ItemType Directory -Path (Join-Path $env:TEMP ("folderflow-demo-" + [guid]::NewGuid()))
$files = New-Item -ItemType Directory -Path (Join-Path $demo.FullName "files")
Set-Content -Path (Join-Path $files.FullName "report.txt") -Value "Sample report"
$demoIndex = Join-Path $demo.FullName "index.txt"

.\build\folderflow.exe --save-index "$($files.FullName)" "$demoIndex"
.\build\folderflow.exe --load-index "$demoIndex"
.\build\folderflow.exe --find-name "$demoIndex" "report"
.\build\folderflow.exe --index-summary "$demoIndex"
.\build\folderflow.exe --check-index "$($files.FullName)" "$demoIndex"
```

The search should show `report.txt` and `Matches: 1`. The summary should
show `Total files: 1`. The final command compares the folder with the
saved index and should show `Changed: 0, Missing: 0`.
