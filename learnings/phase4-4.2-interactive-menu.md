# Phase 4.2 — Interactive console menu

Start FolderFlow without arguments to see the menu. It returns to the menu
after each operation. Enter an existing folder or file where prompted.
Paths with spaces are supported, including paths pasted with double quotes.
Relative paths start from the current working directory.

| Choice | What it does | Equivalent CLI |
| --- | --- | --- |
| 1 | Print the contents and totals of a folder | `--scan <folder>` |
| 2 | Report storage by extension and show the five largest files and folders | `--analyze-storage <folder>` |
| 3 | Find a name in a saved index | `--find-name <index-file> <name-part>` |
| 4 | Preview rule-based file organization | `--dry-run <folder> <config-file>` |
| 5 | Move files according to the rules (Windows) | `--organize <folder> <config-file>` |
| 6 | Undo the latest recorded organization (Windows) | `--undo <folder>` |
| 0 | Exit | |

For choices 4 and 5, press Enter to use `config/extension-rules.txt` from the
repository root, or enter your own rules file. Choices 5 and 6 show the
selected folder and ask you to type `YES`. Any other answer cancels.
The CLI flags remain available for scripts. `--scan` without a folder opens
the original scanner prompt. Other index commands, including creating an index,
are available through the CLI.

## Windows demonstration

Run the commands from the repository root. This creates a fresh temporary
folder containing one document and stores its index separately.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\test.ps1
if ($LASTEXITCODE -ne 0) { throw 'Fix the test failure before continuing.' }

$demo = Join-Path $env:TEMP ('FolderFlow-menu-' + [guid]::NewGuid())
$index = "$demo-index.txt"
New-Item -ItemType Directory -Path $demo | Out-Null
Set-Content -LiteralPath (Join-Path $demo 'report.pdf') -Value 'Disposable demo file'
.\build\folderflow.exe --save-index "$demo" "$index"

@('1', $demo, '2', $demo, '3', $index, 'report', '4', $demo, '', '5', $demo, '', 'NO', '0') |
    .\build\folderflow.exe
Test-Path -LiteralPath (Join-Path $demo 'report.pdf')
```

Expect one file in the scan and storage report, one search match, a preview
for `Documents/report.pdf`, a cancellation message, and `True` at the end.

To demonstrate a confirmed move and undo on the same disposable folder:

```powershell
@('5', $demo, '', 'YES', '0') | .\build\folderflow.exe
Test-Path -LiteralPath (Join-Path $demo 'Documents\report.pdf')
@('6', $demo, 'YES', '0') | .\build\folderflow.exe
Test-Path -LiteralPath (Join-Path $demo 'report.pdf')
```

Both path checks should print `True`. To run the original scanner prompt,
use `$demo | .\build\folderflow.exe --scan`.

## How the code works

`main()` sends explicit command line arguments directly to `runCommand()`.
When launched without arguments, it calls `runMenu()`. The menu reads a line,
matches the number against a small list, and validates the required fields.
It sends the selected command and its arguments to the same dispatcher the CLI
uses. `runFolderScan()` prints a scanner report from an already selected path.
The original scanner prompt calls that same function after reading a path.

The menu searches a fixed list of options. Its work per selection is small
compared with scanning or analyzing a folder. The existing features retain
their own algorithms and handle filesystem errors after dispatch. End of input
closes the menu, and an input stream error returns a failure status.

## Supervisor questions

- **Why use a command callback?** It lets the menu use the existing commands
  and lets tests record what would run without touching files.
- **Why read whole lines?** Folder paths and search terms can contain spaces,
  and the program can reject an entire invalid choice like `1xyz`.
- **How do you prevent accidental moves?** The menu validates the paths and
  requires `YES` before it calls the organizer or undo command.
- **Can old scripts still run?** Existing flags still work. Scripts that piped
  a folder into the program now add `--scan`.
- **Does the menu introduce a new storage algorithm?** No. The existing
  storage analyzer still groups bytes by extension and uses bounded heaps
  to rank the largest paths.
