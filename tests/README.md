# Phase 1 scanner tests

Subphase 1.5, covering 1.1-1.4. Run scripts/test.ps1 from PowerShell.
The script builds all required .cpp files and refuses to run stale executables
after a failed compilation.

The C++ runner reports PASS, FAIL and SKIP separately. Any failure returns 1.
Zero failures returns 0 even when a privileged/unsupported fixture is skipped.
Always report skipped cases beside passed cases.

The 25 named cases cover invalid roots, empty input/EOF, nesting/depth,
.git directory/pointer/root exclusions, external/cyclic/broken links, link roots,
POSIX FIFO, real permission denial, metadata, empty/extensionless files,
Unicode/spaced paths, injected size failure, disappearing metadata, overflow,
repeatable sorted scans, and actual application summaries/status.

Fixtures are generated in exclusively created temporary directories. Cleanup
only targets those owned directories. Windows can block symlink creation.
POSIX FIFO is skipped on Windows. The permission test first probes for real
denial before claiming to exercise denial handling.

Recorded Linux result: 25 passed, 0 failed, 0 skipped. See
../docs/evidence/phase1-1.5-linux.txt. Current Windows counts are not yet known.

Direct Linux reproduction:

    g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror tests/scanner_tests.cpp src/scanner.cpp src/application.cpp src/file_index.cpp src/storage_analyzer.cpp -o build/scanner_tests
    ./build/scanner_tests

Tests use explicit checks, not assertions disabled by NDEBUG.

Phase 2.1 adds `file_index_tests.cpp`: 11 named checks for lookup, replacement,
extension grouping, invalid keys and pointer stability. `scripts/test.ps1` runs
both suites and stops on build failures or failed tests. Windows results are pending.

## Phase 4.1 storage analysis tests

`scripts/test.ps1` also runs `storage_analyzer_tests.cpp`. The suite covers
empty scans, extension grouping, counts and percentages, unknown sizes,
folder totals, bounded rankings and ties, decimal formatting and overflow.
See the [Windows demo](../learnings/phase4-4.1-storage-analysis.md#try-it-on-windows)
for a disposable end-to-end check.

Phase 4.1 Linux verification with GCC 14.2.0 and `-Werror`: scanner 25
passed, index 14 passed and storage analyzer 10 passed, with zero failures
or skips. CLI checks also covered the disposable demo, invalid arguments,
a partially readable folder, and output parity for eight existing index
and rule commands. Phase 4.1 Windows execution is pending.
