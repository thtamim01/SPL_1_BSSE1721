# Development Phase 0 - Foundation

## What exists and why

This is the existing C++17 FolderFlow Guardian project. Its original branches,
commits, scanner and historical documentation remain intact. We did not create
a replacement history.

## 0.1 Git configuration and GitHub connection

Git records local snapshots; GitHub holds remote branches. Status reports the
working tree; fetch updates remote-tracking refs without merging local files.
A clean status alone does not prove a push. The original feature head verified
at this audit was 32bd6a5; main remained b9b9b7f.

## 0.2 Compiler and build setup

g++ -std=c++17 selects the language version. -Wall -Wextra -Wpedantic enable
diagnostics. Each implementation .cpp is a translation unit. Linking combines
their definitions into an executable; a header declares their interfaces.
The new build/test scripts are in scripts/. Current Windows execution remains
pending; the fresh Linux GCC 13.3.0 results are recorded separately.

## 0.3 Structure

src holds production C++; tests holds fixture checks; scripts contains repeatable
commands; docs holds evidence/architecture; learnings explains implemented work.
Build outputs are ignored. No empty future modules were created.

## 0.4 Branch, commit, push and merge

Use develop, verify, integrate, smoke check, document and meaningful commits.
New branches include phase IDs, such as phase1/1.5-modular-scanner. Historical
branch names remain unchanged.

A commit is local; push publishes it. A published feature branch is not
automatically integrated into main. Never discard uncommitted Windows work or
force-push to bypass a mismatch.

## 0.5 Baseline and proposal

C++ project: 7dc19a5; recursive scanner: d5f1be1; git exclusion: b9b9b7f;
storage: 8dca41f; errors: 827958a; docs: 65ba54d; link policy: 32bd6a5.
The original supplied proposal was reread without modifying it and mapped to the
new tracker. Midterm priorities do not remove final-project requirements.

## Evidence, example and demonstration

Inspect the real branch history and docs/PROGRESS_LOG.md, then run the documented
build/test commands. Git is supporting tooling, not a replacement for the
assessed file-management algorithms. Scanner complexity belongs to the Phase 1
manual, not a claim about Git's internal complexity.

Demonstrate a clean build, known-size fixture and the corresponding commit.
Do not describe an unexecuted Windows test as passed.

## Five viva questions

1. Does commit upload? No; push publishes commits.
2. What is a branch? A named reference to a commit.
3. Why stage exact paths? To avoid unrelated files in the commit.
4. Why compile every required .cpp? The linker needs their definitions.
5. Why retain test evidence? To distinguish written code from verified behavior.
