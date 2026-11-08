# Architecture - Phase 1 checkpoint

Subphases: 1.3, 1.4 and 1.5. Implementation: 21c4bf9; tests: 69bb869.

## Components

| File | Responsibility |
| --- | --- |
| src/main.cpp | Start application using standard streams; return its exit status |
| src/application.hpp / .cpp | Selected-path input, entry display, totals, issues and status |
| src/file_metadata.hpp | FileMetadata, EntryKind, ScanIssue and ScanResult value types |
| src/scanner.hpp / .cpp | Selected-root traversal, exclusions, metadata and guarded totals |
| tests/scanner_tests.cpp | Explicit checks and exclusively owned disposable fixtures |
| scripts/build.ps1 | Compile all production translation units; stop after a build failure |
| scripts/test.ps1 | Build application/test runner; execute tests and propagate failure |

The application calls FileScanner::scan and presents the returned ScanResult.
The scanner does not print or read console input. Phase 2 can consume records
directly rather than parsing the terminal output.

## Traversal and metadata

A vector acts as a LIFO stack of pending directories: push_back adds a folder;
back/pop_back remove the last one. This is iterative depth-first traversal.
Opening a directory separately allows a denied child to be recorded while
other pending folders are still scanned; silent permission skipping is avoided.

FileMetadata stores relative path, kind, exact extension, optional byte size
and optional filesystem modification time. Extension case is preserved.
File time is an opaque filesystem clock value, not yet a portable serialized
timestamp or a formatted calendar date.

An absent size is unknown, not zero. Only successful measurements enter the
total. The max-current check prevents unsigned overflow. Counters use uintmax_t.
Records are sorted by generic UTF-8 relative path for stable presentation.
The selected root is not one of its own children. Totals are logical lengths
per path, not physical allocated disk usage; hard links can count more than once.

## Exclusions and failure contract

- Exclude entries named .git, including worktree pointer files.
- Reject selected roots containing a .git component.
- Reject symlink roots; skip discovered file/directory/broken symlinks.
- On Windows, GetFileAttributesW conservatively skips all reparse points,
  including junctions and cloud placeholders. That Windows code path is unverified
  until a Windows build/test. This is the sole platform-specific helper.
- Report other non-regular entries as skipped, not ordinary files.
- Each issue contains path, operation and error text. Any issue makes the result
  incomplete; successful sibling measurements remain available.
- CLI status: 0 complete, 1 invalid/no input, 2 incomplete scan.
- Blank interactive input retains the current-directory default. EOF exits with
  1 instead of silently scanning the working directory.

This scanner is read-only. It is not an atomic snapshot and is not protection
against hostile concurrent link replacement. Mutating modules will require
additional validation. Windows ACL/reparse behavior and console Unicode
input remain platform-verification gates.

## Testability and complexity

FileScanner optionally accepts a FileSizeReader callable. Its default calls
real file_size. Tests supply deterministic failure/overflow behavior without
editing production source. An application overload accepts a scanner to test
the actual partial-result warning and process status.

For N visited entries: O(N) filesystem operations and O(N log N) path comparisons
for sorting. Retained records and pending-directory stack require O(N) records
in the worst case, not merely O(depth). Paths, string lengths and filesystem
latency add costs. No measured performance claim is made.

## Future boundary

Phase 2 adds indexing/search/heap analytics. Phase 3 adds rules, immutable plans,
validated moves and undo. Those modules do not exist yet; no empty future
implementation files were created.

## Phase 2.1 index
`application.cpp` feeds scanned entries to `FileIndex` and displays the indexed count.
The index owns regular-file metadata in a path-keyed map and holds extension buckets
of keys. Each instance describes one scan root. See the [manual](../learnings/phase2-2.1-file-index.md)
for lookup costs, ownership, normalization and limits. Persistence is deferred to 2.2.
