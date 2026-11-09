#pragma once

#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

namespace folderflow
{
using MenuCommand = std::function<int(const std::vector<std::string>&)>;

// A normal exit or end of input returns 0. A stream read error returns 1.
int runMenu(std::istream& input, std::ostream& output, const MenuCommand& execute,
            bool fileMovesAvailable);
}
