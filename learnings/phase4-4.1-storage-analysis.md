# Phase 4.1 — Storage analysis

FolderFlow's scanner already measures the logical size of ordinary files.
The storage report answers a different question: which file types, files
and folders account for those bytes?

## Try it on Windows

From the repository root in PowerShell with g++ on PATH:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\test.ps1
if ($LASTEXITCODE -ne 0) { throw 'Tests failed. Stop before running the demo.' }

$demo = Join-Path $env:TEMP ('FolderFlow-storage-' + [guid]::NewGuid())
New-Item -ItemType Directory -Path (Join-Path $demo 'Media\Raw'), (Join-Path $demo 'Docs'), (Join-Path $demo 'Empty') -Force | Out-Null
[IO.File]::WriteAllBytes((Join-Path $demo 'Media\Raw\video.mp4'), [byte[]]::new(4000000))
[IO.File]::WriteAllBytes((Join-Path $demo 'Media\cover.JPG'), [byte[]]::new(500000))
[IO.File]::WriteAllBytes((Join-Path $demo 'Docs\report.pdf'), [byte[]]::new(500000))
[IO.File]::WriteAllBytes((Join-Path $demo 'README'), [byte[]]::new(0))
.\build\folderflow.exe --analyze-storage "$demo" 3
```

Expect 4 files and 5,000,000 bytes (5.00 MB). The MP4 accounts for 80% and
the JPG and PDF for 10% each. The PDF ranks above the same-sized JPG because
its relative path comes first. The three largest folders are `Media`
(4.50 MB), `Media/Raw` (4.00 MB) and `Docs` (500 KB).

## How it works

- `src/main.cpp` accepts an optional ranking limit from 1 to 100 (default 5).
  `runStorageAnalysis` in `src/application.cpp` scans once and prints the
  results. `analyzeStorage` in `src/storage_analyzer.cpp` works only with
  the metadata the scanner returned, so it can be tested without files.
- A `std::map` holds one counter and byte total per extension. ASCII letters
  are lowercased so `.JPG` and `.jpg` share a bucket. An empty key represents
  a file without an extension. Another map adds each measured file's bytes
  to each ancestor folder below the selected root.
- A binary heap in a `std::vector` holds at most the requested number of
  entries per ranking. Its root holds the weakest retained candidate. A
  better candidate replaces the root, then moves down until heap order is
  restored. Removing the weakest entries and filling the output from the end
  gives largest-first order. Ties use relative path order. The ranking uses
  explicit loops for heap operations.
- File counts include files with unknown sizes. Byte totals, rankings and
  percentages use measured data. A known zero-byte file can appear in the
  ranking. When the measured total is zero, every share displays 0.00%.
  The program reports incomplete scans with exit code 2. If the byte total
  would overflow, it reports an error and stops before printing totals.

With `F` files, `D` directories, `E` extension buckets, `K` ranking
slots, and `A` ancestor visits across measured files, the analyzer uses
`O(F log(E+1) + (D+A) log(D+1) + (F+D) log(K+1))` time, ignoring
path-string processing. It uses `O(E+D+K)` extra entries beyond the
scanner's data. The existing scanner has its own traversal and ordering costs.

Folder totals include descendants, so adding rows for a parent and its child
would double count. The selected root is excluded from the folder ranking.
Sizes are logical bytes per path, not disk allocation or reclaimable storage.
The report follows the scanner's exclusions and uses decimal units
(1 KB = 1,000 bytes). Files can change during a scan.

## References

Phase 4.1 uses standard C++ containers. The existing scanner reads file sizes
through the filesystem library. The ranking code implements the heap operations
directly.

- Microsoft Learn, [C++ filesystem functions](https://learn.microsoft.com/en-us/cpp/standard-library/filesystem-functions).
- Microsoft Learn, [map class](https://learn.microsoft.com/en-us/cpp/standard-library/map-class) and [vector class](https://learn.microsoft.com/en-us/cpp/standard-library/vector-class).
- NIST Dictionary of Algorithms and Data Structures, [Binary heap](https://xlinux.nist.gov/dads/HTML/binaryheap.html) and [sift up](https://xlinux.nist.gov/dads/HTML/siftup.html).

These Microsoft Learn references explain the Windows APIs already used in Phase 3:

- [Reparse point operations](https://learn.microsoft.com/en-us/windows/win32/fileio/reparse-point-operations) and [GetFileAttributesW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfileattributesw) explain the attribute check for junctions and other reparse points.
- [MoveFileExW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw) explains the replacement flag that FolderFlow leaves unset.
- [RemoveDirectoryW](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-removedirectoryw) explains cleanup of ordinary empty directories.
