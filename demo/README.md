# Phase 1 demonstration

Run from the repository root after building:

    $fixture = & .\demo\create_fixture.ps1
    $fixture | .\build\folderflow.exe

The script creates a unique folder under TEMP and returns that path.
It does not modify personal directories or delete existing data.
Included: first.bin 500000 bytes; nested/second.bin 1000000 bytes; an empty folder.
A fake .git/ignored.bin contains another 1000000 bytes but is excluded.
Expected: 2 files, 2 folders, 1500000 bytes, 1500.00 KB, 1.50 MB.

This is a scanner-only demo. Indexing, rules and undo are unimplemented.
This PowerShell script still awaits native Windows execution.
