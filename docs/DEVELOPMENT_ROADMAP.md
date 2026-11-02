# FolderFlow Guardian - Development Roadmap

Authoritative hierarchy: FULL PROJECT -> DEVELOPMENT PHASE -> NUMBERED SUBPHASE -> A-E MICRO-WORKFLOW -> TESTED COMMIT -> PHASE CHECKPOINT.

Audit: 3 November 2026 (Asia/Dhaka). Baseline: `32bd6a5ab11f9216023e91e3cd47cad77415487f` on `feature/storage-analysis`; `main` is `b9b9b7f`. This replaces the earlier 14-phase planning scheme; existing commits and historical notes retain their original names.

## Status rules

Only these states are used: NOT STARTED; IN PROGRESS; IMPLEMENTED — UNVERIFIED; TESTED; INTEGRATED; PUBLISHED; BLOCKED. A higher state applies only to the acceptance scope supported by evidence. PUBLISHED identifies the exact verified remote branch/commit, not completion of every platform test. INTEGRATED names the target integration; main integration is recorded separately. Old manual evidence is identified as historical, not a fresh execution.

A Develop -> B Verify -> C Integrate -> D Smoke Check -> E Document. Commits follow meaningful changes, not a fixed quota. New branch convention: `phaseN/N.M-short-description`. The current checkpoint branch is `phase1/1.5-modular-scanner`, covering the dependent 1.3/1.4 corrections and 1.5 extraction.

## Historical evidence

- 0.1-0.4: prior Windows builds, commits and pushes in this conversation; fresh public clone and branch/history inspection on 3 November. Windows filesystem and environment are not directly accessible here.
- 1.1: `0cd749d`; 1.2: `d5f1be1`; initial 1.3 exclusion: `b9b9b7f`.
- Storage baseline: `8dca41f`; size-error reporting: `827958a`; test/progress notes: `65ba54d`; link/non-regular policy: `32bd6a5`.
- Historical Windows normal fixture: two files, two directories, 1500000 bytes; temporary-copy size fault: one measurement fails and measured total becomes 1000000 bytes.
- Windows symbolic-link creation was blocked by privilege requirements. No Windows link-runtime pass is claimed.

## Full tracker

## DEVELOPMENT PHASE 0 - Foundation

| ID | Feature | Status | Acceptance criterion |
| --- | --- | --- | --- |
| 0.1 | Git configuration and GitHub connection | TESTED | Inspect branch/remotes and recorded successful push. |
| 0.2 | C++17 compiler and build setup | TESTED | Compile the existing source; current Windows rerun pending. |
| 0.3 | Initial project structure | PUBLISHED | Source, docs, README and ignore rules exist. |
| 0.4 | Git branching, commits, pushes, and merges | PUBLISHED | Existing authentic branches and ancestry remain intact. |
| 0.5 | Original repository baseline and proposal preservation | TESTED | Retain original history and supplied proposal; scope mapping below. |

### 0.1 - Git configuration and GitHub connection
- Phase/subphase: 0 / 0.1.
- Status: TESTED.
- Acceptance: Inspect branch/remotes and recorded successful push.
- Test evidence: Historical evidence above; no new runtime execution claimed in this initial tracker.
- Related commits: See historical evidence above.
- Learning manual: Not yet available for this subphase.
- Dependencies: Existing repository and supplied proposal.
- Known problems: Current Windows recheck pending.
- Next action: Reverify current environment when Windows checkout is available.

### 0.2 - C++17 compiler and build setup
- Phase/subphase: 0 / 0.2.
- Status: TESTED.
- Acceptance: Compile the existing source; current Windows rerun pending.
- Test evidence: Historical evidence above; no new runtime execution claimed in this initial tracker.
- Related commits: See historical evidence above.
- Learning manual: Not yet available for this subphase.
- Dependencies: Existing repository and supplied proposal.
- Known problems: Current Windows recheck pending.
- Next action: Reverify current environment when Windows checkout is available.

### 0.3 - Initial project structure
- Phase/subphase: 0 / 0.3.
- Status: PUBLISHED.
- Acceptance: Source, docs, README and ignore rules exist.
- Test evidence: Historical evidence above; no new runtime execution claimed in this initial tracker.
- Related commits: See historical evidence above.
- Learning manual: Not yet available for this subphase.
- Dependencies: Existing repository and supplied proposal.
- Known problems: Current Windows recheck pending.
- Next action: Reverify current environment when Windows checkout is available.

### 0.4 - Git branching, commits, pushes, and merges
- Phase/subphase: 0 / 0.4.
- Status: PUBLISHED.
- Acceptance: Existing authentic branches and ancestry remain intact.
- Test evidence: Historical evidence above; no new runtime execution claimed in this initial tracker.
- Related commits: See historical evidence above.
- Learning manual: Not yet available for this subphase.
- Dependencies: Existing repository and supplied proposal.
- Known problems: Current Windows recheck pending.
- Next action: Reverify current environment when Windows checkout is available.

### 0.5 - Original repository baseline and proposal preservation
- Phase/subphase: 0 / 0.5.
- Status: TESTED.
- Acceptance: Retain original history and supplied proposal; scope mapping below.
- Test evidence: Historical evidence above; no new runtime execution claimed in this initial tracker.
- Related commits: See historical evidence above.
- Learning manual: Not yet available for this subphase.
- Dependencies: Existing repository and supplied proposal.
- Known problems: Current Windows recheck pending.
- Next action: Reverify current environment when Windows checkout is available.

## DEVELOPMENT PHASE 1 - File Scanning and Metadata

| ID | Feature | Status | Acceptance criterion |
| --- | --- | --- | --- |
| 1.1 | Basic directory scanning | PUBLISHED | Validate a selected root and list/count ordinary entries. |
| 1.2 | Recursive scanning | PUBLISHED | Visit nested folders and display relative paths. |
| 1.3 | Smart exclusions | IN PROGRESS | Exclude .git; skip links; report inaccessible entries without presenting complete totals. |
| 1.4 | File metadata and storage | IN PROGRESS | Return type, extension, size and modification metadata; correct decimal KB/MB. |
| 1.5 | Modular scanner and automated tests | NOT STARTED | Separate scanner/UI; execute repeatable nested/invalid/exclusion/size checks. |

### 1.1 - Basic directory scanning
- Phase/subphase: 1 / 1.1.
- Status: PUBLISHED.
- Acceptance: Validate a selected root and list/count ordinary entries.
- Test evidence: Historical evidence above; no new runtime execution claimed in this initial tracker.
- Related commits: 0cd749d
- Learning manual: planned in learnings/phase1-scanning.md; not yet written
- Dependencies: Phase 0 applicable acceptance.
- Known problems: See acceptance gaps and test limitations; no completion inferred from earlier phase names.
- Next action: Finish metadata/error reporting and execute the modular scanner suite.

### 1.2 - Recursive scanning
- Phase/subphase: 1 / 1.2.
- Status: PUBLISHED.
- Acceptance: Visit nested folders and display relative paths.
- Test evidence: Historical evidence above; no new runtime execution claimed in this initial tracker.
- Related commits: d5f1be1
- Learning manual: planned in learnings/phase1-scanning.md; not yet written
- Dependencies: 1.1.
- Known problems: See acceptance gaps and test limitations; no completion inferred from earlier phase names.
- Next action: Finish metadata/error reporting and execute the modular scanner suite.

### 1.3 - Smart exclusions
- Phase/subphase: 1 / 1.3.
- Status: IN PROGRESS.
- Acceptance: Exclude .git; skip links; report inaccessible entries without presenting complete totals.
- Test evidence: Historical evidence above; no new runtime execution claimed in this initial tracker.
- Related commits: b9b9b7f, 32bd6a5 (partial acceptance)
- Learning manual: planned in learnings/phase1-scanning.md; not yet written
- Dependencies: 1.2.
- Known problems: See acceptance gaps and test limitations; no completion inferred from earlier phase names.
- Next action: Finish metadata/error reporting and execute the modular scanner suite.

### 1.4 - File metadata and storage
- Phase/subphase: 1 / 1.4.
- Status: IN PROGRESS.
- Acceptance: Return type, extension, size and modification metadata; correct decimal KB/MB.
- Test evidence: Historical evidence above; no new runtime execution claimed in this initial tracker.
- Related commits: 8dca41f, 827958a (sizes only)
- Learning manual: planned in learnings/phase1-scanning.md; not yet written
- Dependencies: 1.3.
- Known problems: See acceptance gaps and test limitations; no completion inferred from earlier phase names.
- Next action: Finish metadata/error reporting and execute the modular scanner suite.

### 1.5 - Modular scanner and automated tests
- Phase/subphase: 1 / 1.5.
- Status: NOT STARTED.
- Acceptance: Separate scanner/UI; execute repeatable nested/invalid/exclusion/size checks.
- Test evidence: Historical evidence above; no new runtime execution claimed in this initial tracker.
- Related commits: None for complete acceptance.
- Learning manual: planned in learnings/phase1-scanning.md; not yet written
- Dependencies: 1.4.
- Known problems: See acceptance gaps and test limitations; no completion inferred from earlier phase names.
- Next action: Finish metadata/error reporting and execute the modular scanner suite.

## DEVELOPMENT PHASE 2 - Persistent Indexing and Intelligent Search

| ID | Feature | Status | Acceptance criterion |
| --- | --- | --- | --- |
| 2.1 | Index data structures | NOT STARTED | Define file keys/records and justify lookup structures. |
| 2.2 | Persistent indexing | NOT STARTED | Round-trip validated records after restart; identify stale/missing files. |
| 2.3 | Search engine | NOT STARTED | Name/extension and useful metadata filters; empty/invalid query behavior. |
| 2.4 | Storage analytics | NOT STARTED | Heap-based largest-file ranking and extension/storage summaries. |
| 2.5 | Index/search integration and tests | NOT STARTED | Correct persistent reload, search, ranking and documented complexity. |

### 2.1 - Index data structures
- Phase/subphase: 2 / 2.1.
- Status: NOT STARTED.
- Acceptance: Define file keys/records and justify lookup structures.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: Phase 1 applicable acceptance.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 2.2 - Persistent indexing
- Phase/subphase: 2 / 2.2.
- Status: NOT STARTED.
- Acceptance: Round-trip validated records after restart; identify stale/missing files.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 2.1.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 2.3 - Search engine
- Phase/subphase: 2 / 2.3.
- Status: NOT STARTED.
- Acceptance: Name/extension and useful metadata filters; empty/invalid query behavior.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 2.2.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 2.4 - Storage analytics
- Phase/subphase: 2 / 2.4.
- Status: NOT STARTED.
- Acceptance: Heap-based largest-file ranking and extension/storage summaries.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 2.3.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 2.5 - Index/search integration and tests
- Phase/subphase: 2 / 2.5.
- Status: NOT STARTED.
- Acceptance: Correct persistent reload, search, ranking and documented complexity.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 2.4.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

## DEVELOPMENT PHASE 3 - Rule-Based File Organization

| ID | Feature | Status | Acceptance criterion |
| --- | --- | --- | --- |
| 3.1 | Rule configuration | NOT STARTED | Parse initial extension rules with deterministic priority/conflict policy. |
| 3.2 | Rule evaluation engine | NOT STARTED | Match metadata and report destinations, conflicts and unmatched files. |
| 3.3 | Dry-run organization | NOT STARTED | Preview moves; detect collisions/path escapes; do not modify input files. |
| 3.4 | Safe organization | NOT STARTED | Create required folders and move only validated files without overwrites. |
| 3.5 | Undo and empty-folder handling | NOT STARTED | Stack-backed reversal restores paths; cleanup only verified empty folders. |

### 3.1 - Rule configuration
- Phase/subphase: 3 / 3.1.
- Status: NOT STARTED.
- Acceptance: Parse initial extension rules with deterministic priority/conflict policy.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: Phase 2 applicable acceptance.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 3.2 - Rule evaluation engine
- Phase/subphase: 3 / 3.2.
- Status: NOT STARTED.
- Acceptance: Match metadata and report destinations, conflicts and unmatched files.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 3.1.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 3.3 - Dry-run organization
- Phase/subphase: 3 / 3.3.
- Status: NOT STARTED.
- Acceptance: Preview moves; detect collisions/path escapes; do not modify input files.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 3.2.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 3.4 - Safe organization
- Phase/subphase: 3 / 3.4.
- Status: NOT STARTED.
- Acceptance: Create required folders and move only validated files without overwrites.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 3.3.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 3.5 - Undo and empty-folder handling
- Phase/subphase: 3 / 3.5.
- Status: NOT STARTED.
- Acceptance: Stack-backed reversal restores paths; cleanup only verified empty folders.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 3.4.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

## DEVELOPMENT PHASE 4 - Duplicate Detection and Safe Recovery

| ID | Feature | Status | Acceptance criterion |
| --- | --- | --- | --- |
| 4.1 | Duplicate candidate grouping | NOT STARTED | Group by file size and discard impossible matches. |
| 4.2 | Hash-based verification | NOT STARTED | Fingerprint candidates then verify bytes, including forced hash collisions. |
| 4.3 | Duplicate reports | NOT STARTED | Report groups and potential wasted storage without automatic deletion. |
| 4.4 | Managed safe deletion | NOT STARTED | Preserve original paths; move approved test files to recovery; restore safely. |
| 4.5 | Recovery and duplicate regression tests | NOT STARTED | Check nonduplicates, collisions, missing records and restores. |

### 4.1 - Duplicate candidate grouping
- Phase/subphase: 4 / 4.1.
- Status: NOT STARTED.
- Acceptance: Group by file size and discard impossible matches.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: Phase 3 applicable acceptance.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 4.2 - Hash-based verification
- Phase/subphase: 4 / 4.2.
- Status: NOT STARTED.
- Acceptance: Fingerprint candidates then verify bytes, including forced hash collisions.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 4.1.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 4.3 - Duplicate reports
- Phase/subphase: 4 / 4.3.
- Status: NOT STARTED.
- Acceptance: Report groups and potential wasted storage without automatic deletion.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 4.2.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 4.4 - Managed safe deletion
- Phase/subphase: 4 / 4.4.
- Status: NOT STARTED.
- Acceptance: Preserve original paths; move approved test files to recovery; restore safely.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 4.3.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 4.5 - Recovery and duplicate regression tests
- Phase/subphase: 4 / 4.5.
- Status: NOT STARTED.
- Acceptance: Check nonduplicates, collisions, missing records and restores.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 4.4.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

## DEVELOPMENT PHASE 5 - File Versioning and Chunk Deduplication

| ID | Feature | Status | Acceptance criterion |
| --- | --- | --- | --- |
| 5.1 | Version metadata and history | NOT STARTED | Save/list version identifiers and metadata. |
| 5.2 | Fixed-size chunk storage | NOT STARTED | Persist ordered fixed-size chunks and metadata. |
| 5.3 | Chunk reuse and reference counting | NOT STARTED | Reuse only byte-verified equal chunks; maintain correct reference counts. |
| 5.4 | Version reconstruction | NOT STARTED | Reassemble selected versions in order and verify integrity. |
| 5.5 | Cleanup and version tests | NOT STARTED | Safely reclaim unreferenced chunks; test changed/unchanged/corrupt data. |

### 5.1 - Version metadata and history
- Phase/subphase: 5 / 5.1.
- Status: NOT STARTED.
- Acceptance: Save/list version identifiers and metadata.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: Phase 4 applicable acceptance.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 5.2 - Fixed-size chunk storage
- Phase/subphase: 5 / 5.2.
- Status: NOT STARTED.
- Acceptance: Persist ordered fixed-size chunks and metadata.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 5.1.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 5.3 - Chunk reuse and reference counting
- Phase/subphase: 5 / 5.3.
- Status: NOT STARTED.
- Acceptance: Reuse only byte-verified equal chunks; maintain correct reference counts.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 5.2.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 5.4 - Version reconstruction
- Phase/subphase: 5 / 5.4.
- Status: NOT STARTED.
- Acceptance: Reassemble selected versions in order and verify integrity.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 5.3.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 5.5 - Cleanup and version tests
- Phase/subphase: 5 / 5.5.
- Status: NOT STARTED.
- Acceptance: Safely reclaim unreferenced chunks; test changed/unchanged/corrupt data.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 5.4.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

## DEVELOPMENT PHASE 6 - Snapshots and System Reliability

| ID | Feature | Status | Acceptance criterion |
| --- | --- | --- | --- |
| 6.1 | Folder snapshot creation | NOT STARTED | Record selected folder structure and file state. |
| 6.2 | Snapshot comparison | NOT STARTED | Correctly identify added, removed and changed files. |
| 6.3 | Snapshot restoration | NOT STARTED | Preview and validate restore operations and naming conflicts. |
| 6.4 | Failure and integrity handling | NOT STARTED | Detect missing references, incomplete operations and corrupt metadata. |
| 6.5 | Recovery integration tests | NOT STARTED | Execute snapshot restore/failure scenarios and document limits. |

### 6.1 - Folder snapshot creation
- Phase/subphase: 6 / 6.1.
- Status: NOT STARTED.
- Acceptance: Record selected folder structure and file state.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: Phase 5 applicable acceptance.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 6.2 - Snapshot comparison
- Phase/subphase: 6 / 6.2.
- Status: NOT STARTED.
- Acceptance: Correctly identify added, removed and changed files.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 6.1.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 6.3 - Snapshot restoration
- Phase/subphase: 6 / 6.3.
- Status: NOT STARTED.
- Acceptance: Preview and validate restore operations and naming conflicts.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 6.2.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 6.4 - Failure and integrity handling
- Phase/subphase: 6 / 6.4.
- Status: NOT STARTED.
- Acceptance: Detect missing references, incomplete operations and corrupt metadata.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 6.3.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 6.5 - Recovery integration tests
- Phase/subphase: 6 / 6.5.
- Status: NOT STARTED.
- Acceptance: Execute snapshot restore/failure scenarios and document limits.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 6.4.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

## DEVELOPMENT PHASE 7 - Integration, Testing, Documentation and Delivery

| ID | Feature | Status | Acceptance criterion |
| --- | --- | --- | --- |
| 7.1 | Complete regression testing | NOT STARTED | Execute cross-module final regression suite. |
| 7.2 | Performance and reliability | NOT STARTED | Measure larger controlled folders; report actual time/memory limits. |
| 7.3 | Technical documentation | NOT STARTED | Complete build, architecture, tests, learning and proposal comparison. |
| 7.4 | Academic demonstration and viva | NOT STARTED | Reproduce a 3-5-minute demonstration and explain algorithms/limits. |
| 7.5 | Final project release | NOT STARTED | Reproduce final source/build/history and identify unfinished approved scope. |

### 7.1 - Complete regression testing
- Phase/subphase: 7 / 7.1.
- Status: NOT STARTED.
- Acceptance: Execute cross-module final regression suite.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: Phase 6 applicable acceptance.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 7.2 - Performance and reliability
- Phase/subphase: 7 / 7.2.
- Status: NOT STARTED.
- Acceptance: Measure larger controlled folders; report actual time/memory limits.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 7.1.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 7.3 - Technical documentation
- Phase/subphase: 7 / 7.3.
- Status: NOT STARTED.
- Acceptance: Complete build, architecture, tests, learning and proposal comparison.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 7.2.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 7.4 - Academic demonstration and viva
- Phase/subphase: 7 / 7.4.
- Status: NOT STARTED.
- Acceptance: Reproduce a 3-5-minute demonstration and explain algorithms/limits.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 7.3.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

### 7.5 - Final project release
- Phase/subphase: 7 / 7.5.
- Status: NOT STARTED.
- Acceptance: Reproduce final source/build/history and identify unfinished approved scope.
- Test evidence: None; planned work only.
- Related commits: None for complete acceptance.
- Learning manual: Not yet available for this subphase.
- Dependencies: 7.4.
- Known problems: Not implemented.
- Next action: Begin after dependencies and the agreed session checkpoint.

## Proposal coverage and midterm boundary

The supplied final proposal remains the functional authority. This is a user-space selected-directory tool, not an operating-system filesystem. Required scope maps as follows: scanning/metadata -> Phase 1; persistent index/search/storage -> Phase 2; organization/folder creation/rules/empty folders/undo -> Phase 3; duplicates/deletion/recovery -> Phase 4; versions/chunks/reconstruction -> Phase 5; snapshots/integrity -> Phase 6; verification/documentation/delivery -> Phase 7.

The initial Phase 3 rules use extensions. Full-proposal criteria (type, size, name, modification date and current location) remain required later; they are not silently dropped. Recovery expiration policy and interruption-safe journals need explicit designs before destructive operations. GUI/background watching/cloud synchronization are not added to scope.

Custom indexing, heap ranking and undo structures must demonstrate justified academic algorithms. C++ standard filesystem and vectors remain appropriate for scanner plumbing. Hash equality alone never proves duplicate/chunk identity.

The primary midterm checkpoint is Phases 1-3, independently from full-project completion. Later phases remain required for final delivery. See [MIDTERM_READINESS.md](MIDTERM_READINESS.md) and [PROGRESS_LOG.md](PROGRESS_LOG.md).

## Reference engineering review

Read-only review of Rifat Al Maruf (IIT Search Engine) and Raian Huq (CloneScope) on 3 November 2026. Adopt numbered learning indexes, small module interfaces, deterministic fixtures, focused test executables, explicit limitations and evidence/commit links. Do not transfer their search or clone-detection algorithms into this project. Their reported test counts are not FolderFlow test results.

- https://github.com/RIFAT-x7/iit-search-engine
- https://github.com/raian-x8/SPL_01_BSSE1708

Legacy docs/progress.md and docs/testing.md are retained as historical records. This roadmap is the master tracker from this checkpoint onward.
