# FolderFlow Guardian - Midterm Readiness

Budget: four sessions of about two hours (eight hours total). These are relative work sessions, not invented presentation dates. Phases 1-3 are the primary midterm scope. Seven checked items is a target, not a promise or full-project completion percentage.

## Observable ten-item checklist

| # | Demonstration item and proof | Audit status |
| --- | --- | --- |
| 1 | Repeatable build plus nested scan with controlled counts | IN PROGRESS: historical manual build only; scripts pending |
| 2 | Structured metadata, correct exact byte/KB/MB totals | IN PROGRESS: sizes published, complete records pending |
| 3 | Automated scanner tests including invalid roots/exclusions | NOT STARTED |
| 4 | Create a persistent index, restart and reload it | NOT STARTED |
| 5 | Correct filename/extension search with no-match cases | NOT STARTED |
| 6 | Heap-based largest-file results checked against known sizes | NOT STARTED |
| 7 | Load deterministic extension rules and preview destinations | NOT STARTED |
| 8 | Reject collisions and paths escaping the chosen root | NOT STARTED |
| 9 | Safe disposable-folder moves followed by verified undo | NOT STARTED |
| 10 | Published tested checkpoint, README/manuals and rehearsed 3-5-minute demo | IN PROGRESS: some docs published; integrated demo pending |

Verified complete checklist items at audit: 0/10 under these new composite criteria. This does not erase the earlier working scanner/storage features. Every checked item requires linked current evidence; planned work never counts.

## Four-session execution board

| Session | Time box | Priority and exit evidence | Fallback |
| --- | --- | --- | --- |
| 1 | 0-20 min | Reconcile Git baseline and phase tracker | Preserve local work and use a reviewed patch if remote writes fail |
| 1 | 20-100 min | Finish 1.3/1.4 and 1.5 modular scanner with repeatable tests | Keep a tested scanner checkpoint; disclose platform gaps |
| 1 | 100-120 min | Windows smoke/test execution, manual and verified publication | Prepared branch/patch does not count as a verified push |
| 2 | 0-35 min | 2.1 file records/custom lookup and 2.2 narrow versioned index format | Plain validated metadata store; no external database |
| 2 | 35-85 min | Save/reload restart test; basic 2.3 name/extension search | Keep scope narrow; document stale-index semantics |
| 2 | 85-120 min | 2.4 bounded heap ranking, 2.5 regression and learning notes | Defer extra filters before persistence correctness |
| 3 | 0-45 min | 3.1/3.2 extension rules and deterministic priorities | One documented configuration format |
| 3 | 45-100 min | 3.3 dry-run, collisions and root-boundary checks | Stop at reliable preview if mutation safety is not ready |
| 3 | 100-120 min | Tests, docs, coherent commit | Do not rush moves to meet a number |
| 4 | 0-60 min | 3.4/3.5 safe moves and undo only if validation is ready | Demo previews and clearly report unimplemented movement/undo |
| 4 | 60-90 min | Regression fixes and clean Windows build | Freeze last stable implementation |
| 4 | 90-120 min | Evidence, README, learning notes and demo rehearsal | Demonstrate the verified subset; report unmet checklist items |

This is an ambitious budget. Keep Phase 4 out of the eight-hour commitment unless all primary acceptance gates and rehearsal are complete. Do not replace failure testing with extra features.

## Planned 3-5-minute demonstration

1. Scan a disposable fixture; explain relative paths, metadata and totals.
2. Save/reload the index after restart; run a known search and largest-file query if implemented.
3. Load extension rules; preview source/destination and show a rejected collision.
4. If verified, apply test moves and undo; show exact byte preservation.
5. State implemented scope, blocked tests and remaining approved final-project work.

## Full-project separation

Phases 4-6 and final Phase 7 acceptance remain NOT STARTED at audit. Early scanner work is not evidence of duplicate recovery, versioning, chunk reuse or snapshots. No percentage of the four-month project is asserted.
