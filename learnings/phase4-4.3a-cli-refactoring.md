# Phase 4.3A — Modular CLI architecture

At the Phase 4.2 checkpoint, `main.cpp` had 767 lines. It started the console,
handled the menu callback, parsed command arguments, checked rules, managed
indexes, and contained the Windows move and undo code. A change to one console
prompt meant reading a file that also contained recovery and filesystem checks.

| File | Responsibility |
| --- | --- |
| `src/main.cpp` | Pass the process arguments and console streams to `runCli` and return its status. |
| `src/cli_dispatcher.hpp` | Declare `runCli`, the public entry point of the CLI module. |
| `src/cli_dispatcher.cpp` | Select the menu or direct command path, validate arguments, and route the command. The existing organization and undo code lives here for this subphase. |
| `src/menu.cpp` | Display choices, collect answers, and require confirmation before invoking a move or undo. |
| `src/application.cpp` and existing feature files | Scan files, analyze storage, manage index data, or load rules. |

## Follow the execution

1. Start in the eight-line `main.cpp`. `argc` counts arguments including the
   executable name. `argv[0]` is the program name and `argv[1]`, when present,
   is the first command argument.
2. Read `runCli` at the bottom of `cli_dispatcher.cpp`. It calls the private
   `runCommand` directly if the user supplied a command. With no command,
   it starts `runMenu`.
3. In `menu.cpp`, find the scan option. The menu sends a command name and
   arguments back to a callback, a function passed to the menu by `runCli`.
   The callback builds the argument array expected by `runCommand`. Its
   pointers refer to strings that are alive throughout the synchronous call.
4. Read the `--scan` branch in `runCommand`. The branch chooses between the
   original scan prompt and an explicit folder argument. Then trace
   `runFolderScan` in `application.cpp` and see how it uses `FileScanner`.
5. Read the first few cases in `tests/cli_dispatcher_tests.cpp`. They call
   `runCli` with strings as input and capture its output and errors. The
   production executable passes the real console streams into the same function.

The dispatcher compares the command name with a small fixed list of flags and
then calls the matching handler. The cost of a scan or move belongs to that
handler. This subphase introduces no new data processing algorithm.

## Five supervisor questions

1. **Why separate the dispatcher from main?** Main now wires up the executable.
   The dispatcher owns decisions about which command to run. Each file has a
   clear reason to change.
2. **Why do menu and direct CLI commands use the same function?** Both reach
   `runCommand`, so they share argument validation and feature behavior.
3. **Why declare `runCli` in a header?** Main and tests need its signature to
   compile. The linker connects their calls to its definition in
   `cli_dispatcher.cpp`.
4. **How can tests run the CLI without typing?** They pass an input string stream
   and capture output and errors in separate string streams, then check both
   the messages and the returned status.
5. **Where are the Windows safety checks?** They moved with the original
   handlers into `cli_dispatcher.cpp`. The `_WIN32` blocks still check
   reparse points and destination conditions and use the existing move journal.

## Windows verification

Run these commands from the repository root after switching to the Phase 4.3A
branch. The test script compiles the executable and all five suites. The
dispatcher tests create disposable fixtures and exercise real move and undo
on Windows. The manual checks below use a second temporary fixture.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\test.ps1
if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }

$demo = powershell -NoProfile -ExecutionPolicy Bypass -File .\demo\create_fixture.ps1
if ($LASTEXITCODE -ne 0) { throw 'Demo setup failed.' }

.\build\folderflow.exe --scan "$demo"
.\build\folderflow.exe --analyze-storage "$demo" 3
@('1', $demo, '0') | .\build\folderflow.exe
```

The direct and menu scans should show the same fixture files and summary.
Storage analysis should report 1,500,000 measured bytes. Windows results should
be recorded after running the commands. Some existing scanner fixtures can skip
when the system cannot create links or reproduce a permission denial.

## References

- [Microsoft Learn — Header files](https://learn.microsoft.com/en-us/cpp/cpp/header-files-cpp)
- [Microsoft Learn — Translation units and linkage](https://learn.microsoft.com/en-us/cpp/cpp/program-and-linkage-cpp)
- [Microsoft Learn — Main function and command-line arguments](https://learn.microsoft.com/en-us/cpp/cpp/main-function-command-line-args)
