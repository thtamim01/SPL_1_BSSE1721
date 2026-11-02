# Phase 2.1 — In-memory file index

## 1. What was built
`FileIndex` adds or replaces regular-file records, finds a record by relative path,
and groups paths by extension. The application indexes scanner records and prints
`Indexed Files`. Directories and skipped links are rejected.

## 2. Why
Later search and persistence need records that can be addressed consistently.
`nested/report.txt` and `other/report.txt` must remain separate records.

## 3. How it works
`filesByPath_` is a `std::map<string, FileMetadata>` owning the records.
`pathsByExtension_` is a `std::map<string, vector<string>>` holding their path keys.
`add` inserts a new key or updates the existing metadata. An extension change removes
the old bucket membership. Adding the same record twice never duplicates it.

## 4. C++ concepts and reading order
Read `src/file_index.hpp`, then `src/file_index.cpp`, then the index lines in
`src/application.cpp`. A class hides its maps behind public functions. A const
pointer permits reading a record without copying it; `nullptr` means absent.
The pointers refer to this index's records and must not outlive it. Map insertion
preserves existing pointers. Replacing metadata updates the same record.
`optional` preserves the difference between an unknown size and a measured zero.

## 5. Data structures and algorithms
`std::map` provides ordered keys and logarithmic lookup. Its typical implementation
is a balanced tree; the standard guarantees the complexity, not a particular tree.
A vector is a compact sequence of keys for one extension. No tree algorithm is
claimed as handwritten. Future heap ranking belongs to 2.4.
Path keys use lexical normalization and UTF-8 generic separators. Paths remain
case sensitive. Empty, absolute and parent-escaping normalized keys are rejected.
Extensions use ASCII case folding and accept an optional leading dot.

## 6. Complexity
Let n be files, e extension groups and k matches. Ignoring string lengths:
new insertion is O(log n + log e), with amortized vector append.
Path lookup is O(log n). Extension query is O(log e + k log n), because each
path key is resolved through the owning map. Replacement with a changed extension
also scans the old bucket, taking O(k) there. Storage is O(n) records and path keys.
The previous chat's simplified O(log n) insertion explanation omitted bucket update
cost and string costs; use these bounds in the viva.

## 7. Example
After adding `notes/a.TXT` and `other/a.txt`, querying `txt` returns two records.
Querying `notes/a.TXT` returns one. Querying an absent path returns nullptr.
The known scanner fixture displays `Indexed Files: 2`.

## 8. Tests and limits
Native Linux evidence: 11 index tests passed; 25 scanner/application tests passed.
See `docs/evidence/phase2-2.1-linux.txt`. Windows verification is pending.
Tests cover duplicate names, replacement, bucket updates, missing queries, unsafe
keys, extensionless files, missing sizes, normalized paths and pointer stability.
The integration regression runs the real application through streams.
This index belongs to one scan root; records are snapshots, not live file handles.
Extension results follow insertion order. No persistent saving, filename query UI,
Unicode case folding or stale-file detection is implemented in 2.1.
A partial scan produces a partial index and the existing scan warning remains.

## 9. Supervisor questions
Why path keys? Names can repeat in different directories.
Why two maps? One owns records; another groups references for extension lookup.
Why a vector in each bucket? Several files share one extension.
Does indexing read file contents? No, it stores scanner metadata.
What happens after restart? The in-memory index disappears until 2.2 exists.

## 10. Demonstration
Run `powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\test.ps1`.
Check both RESULT summaries and the process exit code. Then run the demo commands
in `demo/README.md`; the program now also prints the indexed-file count.
Explain the three functions `add`, `findByPath`, `findByExtension` before publishing.
