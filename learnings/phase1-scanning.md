# Development Phase 1 - File Scanning and Metadata

Implementation: 21c4bf9. Automated suite: 69bb869.
Read the named source and tests alongside each section. Original scanner
implementations remain in Git history.

## Shared C++ concepts

| Construct | Meaning here |
| --- | --- |
| namespace folderflow | Groups project names |
| namespace fs = std::filesystem | Short alias; does not create a filesystem |
| struct | Groups related values such as one file's metadata |
| class FileScanner | Public scan operation with a private size-reader dependency |
| enum class EntryKind | Scoped names for regular files, folders and skipped entries |
| const fs::path& | Pass a path by reference without copying/modifying it |
| std::optional<T> | A value may be absent; absence differs from numeric zero |
| std::error_code& | A checked result from non-throwing filesystem overloads |
| std::uintmax_t | Wide unsigned integer; still needs overflow checks |
| std::vector | Owns a growable array; also used as a LIFO pending stack |
| lambda | Small callable written where needed, such as visiting one entry |
| std::function | Holds the compatible file-size callable used by the scanner |
| std::move | Transfers record members; does not move a file on disk |
| header / implementation | .hpp declares interfaces; .cpp defines behavior |
| stream | Input/output interface; tests substitute in-memory text streams |

For optional sizeBytes, checking the optional tests presence; dereferencing it
reads the number. A present value of zero means a successfully measured empty
file. A missing value means measurement was unavailable.

## 1.1 Basic directory scanning

### What, why and how

Accept a selected root, validate it, list/count children and report invalid
input. A nonexistent root, file root, empty API path, protected .git root and
link root are rejected. The selected root itself is not counted.

runApplication reads one line. An entered blank line preserves the old
current-directory default. EOF is not a blank line: it exits without scanning.
FileScanner::scan resolves the selected path, checks symlink_status and sets
rootValid only after validation.

### Code, algorithm and complexity

Directory iteration supplies child paths. EntryKind controls counting.
Validation uses a bounded set of filesystem queries plus path processing;
enumerating K immediate children costs O(K) entry operations. The current
scanner additionally descends recursively; the original immediate scanner
remains at 0cd749d.

### Example, tests and limitations

An empty folder gives zero files/folders/bytes. A file selected as root produces
Invalid folder path! and exit 1. Tests check empty roots, nonexistent roots,
file roots, empty API paths and EOF. Windows console/ACL behavior is not fully
validated by those cases.

### Viva and demonstration

Why distinguish EOF from blank? Blank explicitly selects the default; EOF
should not silently begin a scan.
Why not count the root? Totals describe entries inside it.
Demonstrate an empty folder followed by a nonexistent path.

## 1.2 Recursive scanning

### What, why and how

Visit nested folders so records represent the selected tree. Relative paths
show each entry's location without repeating the root.

The pending vector starts with the root. Each loop reads its back element,
pops it, opens that directory and pushes ordinary child folders. The last
pushed folder is visited next: this is iterative depth-first traversal.
There is no recursive C++ call for each folder level.

### Code, algorithm and complexity

push_back/back/pop_back make the vector a LIFO stack, not a queue. The result
vector separately retains metadata. Sorting generic relative paths makes
presentation independent of filesystem iteration order.

For N entries, traversal uses O(N) filesystem operations; sorting adds
O(N log N) path comparisons. Retained records and pending directories require
O(N) records in the worst case, plus path/string lengths.

### Example, tests and limitations

A root first.bin and nested/second.bin produce those root-relative records.
The CLI now uses forward slashes consistently on all platforms.
Tests exercise 12 levels and repeated 100-file scans. The 100-file test is a
correctness case, not a performance benchmark. Concurrent changes can make
results incomplete; scanning is not a transactional snapshot.

### Viva and demonstration

Why sort? Directory iteration order is unspecified.
Why use an explicit stack? It avoids per-level recursive call-stack use.
Show the nested fixture and trace how its folder enters/leaves the stack.

## 1.3 Smart exclusions and errors

### What, why and how

Exclude .git directories and pointer files. Skip links and special entries.
Report inaccessible content instead of silently claiming a complete scan.

Check the entry name first, then symlink_status without intentionally following
links. Only ordinary directories enter the pending stack. A Windows-only
GetFileAttributesW helper conservatively skips all reparse points, including
junctions and cloud placeholders. This platform-specific path still needs
Windows build/runtime evidence.

A directory open failure creates a ScanIssue and leaves other pending folders
available. Unlike the previous skip_permission_denied option, it is disclosed.
ScanResult::complete requires a valid root and no issues.

### Algorithm, complexity and example

Exclusion is a bounded name/type decision per entry, apart from path length
and filesystem queries. A cycle link is not pushed. Under a stable filesystem,
it cannot cause endless traversal.
A 1000000-byte .git fixture file does not change the visible 1500000-byte total.
A denied child produces an incomplete result but readable siblings still count.

### Tests and limitations

Linux checks passed for external file links, external/cyclic directory links,
broken links, root links with trailing separators, FIFO skipping and real
permission denial. Historical Windows link creation was blocked by privileges.
The suite reports unavailable fixtures as SKIP, not PASS.

Scanning is read-only and path-based. Hostile concurrent link replacement is
not prevented. Windows junction/reparse and ACL behavior remain verification
gates. Later mutating modules require stronger operation validation.

### Viva and demonstration

Why not use else after is_directory? Other entry types also exist.
Why avoid silent skipping? Otherwise a partial total could appear complete.
Run the suite and distinguish PASS, FAIL and SKIP.

## 1.4 File metadata and storage

### What, why and how

Return type, relative path, extension, optional size and optional modification
time. Phase 2 needs records rather than parsed terminal text.

A successful size read engages sizeBytes. A failed read leaves it absent,
adds an issue and contributes no sentinel value to the total. Modification
time is checked independently. It is still a filesystem clock value, not
yet a portable serialized timestamp or displayed calendar date.

Before adding bytes, check whether bytes exceeds max_uintmax minus totalBytes.
If so, report overflow and do not perform the addition. Unsigned arithmetic
does not automatically prevent wrapping.

### Algorithm, complexity and worked example

Metadata uses O(1) filesystem queries per entry, ignoring operating-system cost.
500000 + 1000000 = 1500000 bytes = 1500.00 KB = 1.50 MB.
If the first measurement fails, the measured total is 1000000 bytes,
1000.00 KB and 1.00 MB. Two files were still discovered.

fixed and setprecision(2) control decimal display. Dividing by 1000.0 and
1000000.0 avoids integer-only division. Binary units would be KiB and MiB.

### Tests and limitations

Tests cover extension case, zero-byte/extensionless files, modification presence,
Unicode filenames, failed-size sentinel exclusion, disappearing metadata and
overflow. Total bytes are per-path logical lengths, not physical allocation.
Multiple hard-link paths may count the same physical data more than once.

### Viva and demonstration

Why optional rather than zero on error? A genuine empty file has known size zero.
Does uintmax_t prevent overflow? No; addition is checked.
What does the file count mean after a size failure? Discovered ordinary files.
Show the normal fixture and the injected-error result.

## 1.5 Modular scanner and automated tests

### What, why and how

main starts the program; application handles console input/output; scanner
returns records. Later index/rule code can reuse data without terminal parsing.

FileScanner's default reader calls real file_size. A test can supply a
compatible callable to simulate a failure or boundary. An application overload
accepts that scanner so tests exercise the actual warning and exit status.
This is dependency injection, not editing production source during a test.

### Test algorithm and important code

TestSuite::run distinguishes failure exceptions from SkipTest. Checks throw
runtime_error explicitly, so NDEBUG does not disable them.
TempDirectory uses create_directory to obtain an exclusively owned unique
fixture. Cleanup targets that owned directory, never an arbitrary user folder.

For T cases, dispatch overhead is O(T), plus actual fixture/scanning work.
The runner prints each actual result and computes passed/failed/skipped counts.
The CLI returns 0 for complete output, 1 for invalid/no input, 2 for partial output.

### Tests, example and limitations

The recorded Linux/GCC 13.3.0 run in docs/evidence/phase1-1.5-linux.txt produced
25 passed, 0 failed, 0 skipped. Both executables compiled with warnings as errors.
The real main executable passed the known fixture and invalid-root smoke.

PowerShell and Win32-only code were reviewed but not executed here. Windows
may skip tests requiring unavailable privileges or POSIX facilities. Do not
report the Linux count as a Windows result.

### Demonstrate

From the Windows repository root, run scripts/build.ps1 and scripts/test.ps1.
Create a disposable sample with demo/create_fixture.ps1 and pipe the returned
path into build/folderflow.exe. Read PASS/FAIL/SKIP separately. Explain one
negative test, then show that main.cpp contains no traversal logic.

### Five likely supervisor questions

1. Why separate components? Startup, presentation and filesystem work can change,
   be tested and be reused through focused interfaces.
2. Why a class and structs? Scanner encapsulates behavior/dependency; records are
   simple related values rather than unnecessary object hierarchies.
3. Why inject a size reader? It reproduces errors reliably without real permission
   changes or source rewriting.
4. Complexity? O(N) filesystem work, O(N log N) path comparisons, O(N) records and
   pending storage, with path-length and filesystem-latency costs.
5. Is it published/integrated? Inspect the exact refs/progress log. A local tested
   commit alone proves neither a push nor a merge into main.
