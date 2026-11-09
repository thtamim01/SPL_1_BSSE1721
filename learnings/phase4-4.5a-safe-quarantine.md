# Phase 4.5A — Safe quarantine and recovery

This phase moves **one explicitly selected regular file** into a reserved folder
under the selected root. It does not choose duplicates, delete files, or alter
the existing organization undo journal. The executable asks for exactly `YES`
before a quarantine or restore, whether invoked directly or through the menu.

## Try it in Windows PowerShell

Run from the repository root. These commands create a disposable folder under
TEMP; replace nothing in your personal folders. Keep the folder until you have
checked the final restored file.

```powershell
git status --short --branch
git log -1 --oneline
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\test.ps1
$demo = Join-Path $env:TEMP ('FolderFlow-quarantine-' + [guid]::NewGuid())
New-Item -ItemType Directory -Path $demo | Out-Null
Set-Content -LiteralPath (Join-Path $demo 'report.txt') -Value 'Disposable report'
$before = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $demo 'report.txt')).Hash
'NO' | .\build\folderflow.exe --quarantine $demo 'report.txt'
Test-Path -LiteralPath (Join-Path $demo 'report.txt')
$move = 'YES' | .\build\folderflow.exe --quarantine $demo 'report.txt'
$move
$id = [regex]::Match(($move -join ' '), 'ID: ([0-9a-f]{32})').Groups[1].Value
if (-not $id) { throw 'No recovery ID: stop and inspect the output.' }
.\build\folderflow.exe --list-quarantine $demo
.\build\folderflow.exe --scan $demo
Set-Content -LiteralPath (Join-Path $demo 'report.txt') -Value 'Different temporary content'
'YES' | .\build\folderflow.exe --restore-quarantine $demo $id
Get-Content -LiteralPath (Join-Path $demo 'report.txt')
Move-Item -LiteralPath (Join-Path $demo 'report.txt') -Destination (Join-Path $demo 'conflicting-report.txt')
'YES' | .\build\folderflow.exe --restore-quarantine $demo $id
$after = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $demo 'report.txt')).Hash
if ($before -ne $after) { throw 'Restored content differs.' }
.\build\folderflow.exe --list-quarantine $demo
.\build\folderflow.exe
```

The first answer cancels without a move. The first restore refuses to replace
the newly created report. The second restore uses the saved ID and succeeds.
In the menu, use 8 to quarantine a relative file path, 9 to list records, and
10 to restore an ID. `0` exits. Do not try the feature on important data before
independently verifying it on your Windows machine.

## Layout and steps

For selected root `R`, relative path `notes/report.txt`, and unique hex ID
`abc...`, files live at `R/.folderflow-quarantine/abc.../payload` and
`R/.folderflow-quarantine/abc.../record.txt`. Keeping the quarantine under the
same root keeps ordinary source and destination on the same volume and keeps
the recovery data with the selected folder. Moving the entire root preserves
the relative original path. The directory consumes space within that root;
people who remove or edit it can make recovery impossible. Treat it as data.

`record.txt` contains a version line, quoted original relative path, quoted
stored relative path, original logical byte count, and append-only status
lines. `PENDING` is flushed before moving; `STORED` follows the move.
`RESTORE_PENDING` precedes restoration; `RESTORED` follows it. IDs use
16 random bytes displayed as 32 lowercase hexadecimal characters; a new
directory is created exclusively, so an ID collision reserves nothing and
another ID is tried. The saved path and size are checked again on recovery.

`quarantineFile` validates every directory component, the selected ordinary
file, and the relative path; rejects symbolic links and Windows reparse points;
reserves an ID; syncs the initial record; then moves without replacement.
`listQuarantine` checks every entry and reports damaged, missing, and incomplete
entries explicitly with a nonzero exit code. `restoreQuarantine` checks the
record and payload, requires the original parent directory, refuses any
occupied original path, records intent, and moves without replacement. The
scanner excludes `.folderflow-quarantine` entirely; organization, duplicate
detection, storage analysis and saved indexes inherit that exclusion.

This implementation uses Windows `MoveFileExW` without `COPY_ALLOWED` or
`REPLACE_EXISTING`, and Linux `renameat2` with `RENAME_NOREPLACE`. It does not
fall back to copy and delete. A cross-volume move fails, leaving the file at
the source, unless an independently created mount or directory replacement
interferes. Other platforms fail closed for moves. `FlushFileBuffers` syncs
record data on Windows; Linux `fsync` also syncs record and parent directories.

## Interrupted operations and limits

After a crash between metadata and moving, a `PENDING` record remains with
the original file. After the move, a `PENDING` record may have a payload;
the list reports the incomplete state and restore can use that valid record.
After `RESTORE_PENDING`, the payload may be present, or the original file may
already have been restored. In the latter case the program refuses a second
move and reports the inconsistency for manual inspection. It never silently
replaces a path or erases a damaged journal. If metadata cannot be synced
before a move, that move is not attempted. If a later status append fails,
the operation returns an error with its recovery ID.

The operation is **not** an atomic transaction across file contents, directory
entries and journal. Windows power-loss durability of newly created directory
entries is not guaranteed by `FlushFileBuffers` on the record alone. Concurrent
writers, hostile replacement of path components after validation, network
shares, unusual filesystems, permissions, and abrupt power loss need manual
review. The byte count is a sanity check, not a cryptographic integrity
proof; do not rely on it against deliberate tampering. Recovery only restores
into an existing safe original parent folder. An occupied target must be
resolved manually without overwriting it. Do not rename the root while
operating on it.

## Code map and cost

| Function | Purpose |
| --- | --- |
| `runMenu` in `menu.cpp` | Presents numbered options and gathers paths/ID. |
| `runCommand` in `cli_dispatcher.cpp` | Demands `YES`, then calls the quarantine module. |
| `quarantineFile` | Checks paths, writes record, and moves one selected file. |
| `listQuarantine` | Checks each ID's record and payload before listing it. |
| `restoreQuarantine` | Rejects occupied paths and restores the selected ID. |
| `FileScanner::scan` | Excludes private quarantine data from normal scanning. |

For each selected path, checking components takes O(depth); the native move
usually updates directory entries without reading the file into application
memory. Listing N records takes O(N plus the total length of their paths).
Application memory per record is O(path length), plus output buffering if a
caller chooses a string stream. Filesystems may implement rename differently;
no constant-time or atomic durability guarantee is made across all devices.

## Questions your supervisor may ask

1. **Why store a record before moving?** The original location and ID remain
   available even if the process stops just after the move.
2. **Why does the ID have to be unique?** Each quarantined file needs its own
   private payload and record; exclusive directory creation handles collisions.
3. **What if a file already exists at the original location?** Restoration
   refuses to overwrite it and keeps the payload for manual resolution.
4. **Why exclude quarantine from scanning?** Otherwise reports and organizer
   could treat internal payloads as ordinary user files.
5. **What if the machine loses power after a move?** The state can be incomplete;
   list reports it, and recovery checks both record and payload. A complete
   power-loss transaction is beyond this phase's guarantees.

## Learning references

- [Microsoft MoveFileExW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw): copy-across-volume and replace flags.
- [Microsoft GetFileAttributesW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileattributesw): reparse attributes and link behavior.
- [C++17 filesystem symlink_status](https://en.cppreference.com/w/cpp/filesystem/status): inspect a path without following the last symbolic link.
- [Linux renameat2](https://man7.org/linux/man-pages/man2/rename.2.html): `RENAME_NOREPLACE` semantics on supported Linux filesystems.

These references explain APIs, not evidence of Windows execution. Windows
verification remains a separate, local step.
