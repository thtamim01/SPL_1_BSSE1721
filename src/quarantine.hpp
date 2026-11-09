#pragma once

#include <filesystem>
#include <iosfwd>
#include <string>

namespace folderflow
{
// Return 0 on success, 1 on rejected input/failure, 2 on incomplete records.
int quarantineFile(const std::filesystem::path& root, const std::filesystem::path& relative,
                   std::ostream& output, std::ostream& errors);
int listQuarantine(const std::filesystem::path& root,
                   std::ostream& output, std::ostream& errors);
int restoreQuarantine(const std::filesystem::path& root, const std::string& id,
                      std::ostream& output, std::ostream& errors);
}
