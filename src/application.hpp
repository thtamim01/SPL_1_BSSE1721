#pragma once

#include <cstddef>
#include <filesystem>
#include <iosfwd>

namespace folderflow
{
class FileScanner;
// 0: complete report; 1: invalid root; 2: partial scan or byte overflow.
int runStorageAnalysis(const std::filesystem::path& root, std::size_t limit,
                       std::ostream& output, std::ostream& errors);
// 0: complete scan; 1: invalid input/root; 2: partial scan with reported issues.
int runApplication(std::istream& input, std::ostream& output);
int runApplication(std::istream& input, std::ostream& output, const FileScanner& scanner);
}
