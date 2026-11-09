#pragma once

#include <iosfwd>

namespace folderflow
{
// Uses main-style arguments: argv[0] is the program name.
// With no command, starts the interactive menu.
int runCli(int argc, const char* const argv[], std::istream& consoleInput,
           std::ostream& consoleOutput, std::ostream& consoleErrors);
}
