# Phase 1 viva notes

Implementation 21c4bf9; tests 69bb869. Answers describe implemented scope.

1. What exists today? A modular selected-directory scanner returning metadata,
   exclusions, measured logical sizes and issues. Later features remain planned.
2. Why C++17 filesystem? Standard paths/iteration/metadata without a heavy framework.
3. Why separate scanner and application? Results can be tested/reused without console text.
4. Traversal algorithm? Iterative depth-first search with a vector as a LIFO stack.
5. Why report permission failures? Otherwise partial totals could look complete.
6. Why optional size? Unknown differs from a known zero-byte file.
7. Why guard unsigned addition? Unsigned arithmetic can still wrap.
8. How are failures tested? An injected size reader reproduces boundaries; actual
   permission/link fixtures are separate platform-dependent tests.
9. Complexity? O(N) filesystem work, O(N log N) path comparisons, O(N) retained/pending
   records, plus path lengths and filesystem latency.
10. Which tests ran? 25 passed on Linux. New Windows execution is pending.
11. Is link safety complete? Windows-specific handling needs native validation;
    path checks do not prevent hostile concurrent replacement.
12. Does a local commit mean published? No. Publication requires remote verification;
    a published branch is also distinct from integration into main.

Practice the known 1500000-byte fixture and explain the injected-failure test
and partial-scan exit code 2. See learnings/ for the longer walkthrough.
