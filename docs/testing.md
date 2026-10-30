# FolderFlow Guardian - Testing

## Storage analysis checkpoint

Date: 31 October 2026
Source commit: 827958a
Environment: Windows, PowerShell, GCC/G++, C++17

## Build command

g++ -std=c++17 -Wall -Wextra -Wpedantic src/main.cpp -o build/folderflow.exe

Result: BUILD SUCCESSFUL.

## Test 1: Normal storage calculation

Controlled fixture:
- first.bin: 500000 bytes.
- nested/second.bin: 1000000 bytes.
- An empty subfolder.
- A .git folder containing an excluded test file.

Observed result:
- Files: 2.
- Folders: 2, excluding the selected root and .git.
- Total: 1500000 bytes.
- Decimal KB: 1500.00.
- Decimal MB: 1.50.
- No size-error warning.

Result: PASS.

## Test 2: Simulated size-measurement failure

A temporary source copy simulated a permission-denied error when
measuring first.bin. The production source was not changed by this test.

Observed result:
- first.bin displayed a size-unavailable message.
- Files: 2.
- Successfully measured total: 1000000 bytes.
- Decimal KB: 1000.00.
- Decimal MB: 1.00.
- Summary warning reported one failed measurement.

Result: PASS.

This verifies the reporting logic through fault injection.
It does not establish behavior under actual Windows permission failures.

## Remaining checks

- Non-regular entries and symbolic links.
- Actual filesystem failures and interrupted traversal.
- Larger directory trees.

All fixtures used disposable test directories.