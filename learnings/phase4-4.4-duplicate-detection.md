# Phase 4.4 — Content-based duplicate detection

Run `folderflow --find-duplicates <folder>`, or choose **7. Find duplicate
files** in the menu. The report contains only confirmed groups. No file is
moved or removed. One copy per group is kept in the hypothetical savings
calculation. The savings are *logical file bytes*, not physical disk space.
Hard links, compression, sparse files and filesystem block allocation can
make physical savings different.

## How the detector works

1. `FileScanner::scan` discovers regular files and excludes links and
   unsuitable roots. The detector carries scanner issues into its report.
2. `std::map` groups paths by measured byte size. A size bucket with only
   one file requires no content reads. Missing size produces an issue.
3. Each file in a candidate size bucket is checked for a regular, unlinked
   path and unchanged size/time. It is read in 64 KiB buffers for a 64-bit
   FNV-1a content hash. Files with different hashes cannot be identical
   under normal unmodified reads. A hash match is only a filter.
4. Within each matching hash bucket, compare every candidate against a
   representative of each confirmed group using buffered **byte-by-byte**
   reads. Keep separate groups when hashes collide but contents differ.
5. Check file metadata again after reading and before reporting. A failed
   scan, read, comparison or changed file yields an `UNVERIFIED` message and
   exit status 2. An invalid root returns 1. A complete report returns 0.
   The report skips an affected group if its members change before output.

For N scanned entries, the size and hash maps require O(N log N) comparisons
in the worst case. Hashing reads candidate content once. Verification reads
the compared files again. Hash collisions or many equal-size distinct files
can require O(K² × S) bytes read in a bucket of K files of size S. Retained
metadata and groups use O(N) memory. The two comparison buffers total 128 KiB.
The hash buffer adds 64 KiB when hashing. Actual operations also incur
filesystem latency.

FNV-1a is a simple noncryptographic 64-bit rolling hash with no extra
dependencies. It is not collision-resistant and never proves equality.
A test deliberately forces every hash to the same value and still finds
only byte-identical files. The detector checks that paths are not links or
Windows reparse points before opening. It rechecks size and modification time,
but another process can race between these checks and a file open or write
the same-size bytes while retaining the timestamp. This is a best-effort
read-only report, not an atomic filesystem snapshot.

## Example

```text
Duplicate group (4 bytes each):
  - A/first.bin
  - B/second.bin
Confirmed groups: 1
Potential reclaimable logical bytes: 4
```

The detector never treats names as proof. A zero-byte group has zero possible
reclaimable bytes. If a candidate cannot be checked, look for `UNVERIFIED:`
on standard error and rerun after resolving that issue.

## Windows PowerShell verification

From the repository root on the feature branch:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\test.ps1
if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }

$demo = Join-Path $env:TEMP ('FolderFlow-duplicates-' + [guid]::NewGuid())
New-Item -ItemType Directory -Path (Join-Path $demo 'A'), (Join-Path $demo 'B') -Force | Out-Null
Set-Content -LiteralPath (Join-Path $demo 'A\first.bin') -Value 'same' -NoNewline
Set-Content -LiteralPath (Join-Path $demo 'B\second.bin') -Value 'same' -NoNewline
Set-Content -LiteralPath (Join-Path $demo 'different.bin') -Value 'else' -NoNewline

$before = Get-ChildItem -LiteralPath $demo -Recurse -File |
    ForEach-Object { '{0}|{1}' -f $_.FullName, (Get-FileHash -LiteralPath $_.FullName).Hash }
.\build\folderflow.exe --find-duplicates "$demo"
@('7', $demo, '0') | .\build\folderflow.exe
$after = Get-ChildItem -LiteralPath $demo -Recurse -File |
    ForEach-Object { '{0}|{1}' -f $_.FullName, (Get-FileHash -LiteralPath $_.FullName).Hash }
if (@(Compare-Object $before $after).Count -ne 0) { throw 'Input files changed.' }
Get-ChildItem -LiteralPath $demo -Recurse | Select-Object FullName
```

The two reports should each show one group containing A/first.bin and
B/second.bin, with 4 potential reclaimable logical bytes. The test script
compiles and runs six suites. Actual Windows counts must be recorded from
the local run. Some scanner tests can skip unavailable link or permission
fixtures.

## Five viva questions

1. **Why group by file size?** Equal content always has equal length, so
   different sizes can be excluded without reading content.
2. **Why hash before comparing bytes?** It cheaply narrows the candidate
   comparisons. Collisions make final byte comparison necessary.
3. **What is a hash collision?** Different bytes producing the same
   fixed-width hash value. The forced-collision test checks that they are
   still reported separately.
4. **How do you calculate potential savings?** For a confirmed group with
   M files of S bytes, keeping one gives (M − 1) × S logical bytes.
5. **How does the feature stay safe?** It uses the scanner's exclusions,
   checks paths and metadata around reads, reports unverifiable files,
   and only opens content for reading. The CLI exposes no deletion action.

## References

- [cppreference — std::filesystem::symlink_status](https://en.cppreference.com/cpp/filesystem/status)
- [cppreference — std::basic_istream::read](https://en.cppreference.com/old/wiki/io/basic_istream/read)
- [IETF draft — FNV hash](https://www.rfc-editor.org/rfc/rfc9923.html)
