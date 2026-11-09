# Phase 1 demonstration

Run from the repository root after building:

    $fixture = & .\demo\create_fixture.ps1
    $fixture | .\build\folderflow.exe --scan

The script creates a unique folder under TEMP and returns that path.
It does not modify personal directories or delete existing data.
Included: first.bin 500000 bytes; nested/second.bin 1000000 bytes; an empty folder.
A fake .git/ignored.bin contains another 1000000 bytes but is excluded.
Expected: 2 files, 2 folders, 1500000 bytes, 1500.00 KB, 1.50 MB.

This demo covers the scanner. For the menu demo, see
[Phase 4.2](../learnings/phase4-4.2-interactive-menu.md).
