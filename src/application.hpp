#pragma once

#include <iosfwd>

namespace folderflow
{
class FileScanner;
// 0: complete scan; 1: invalid input/root; 2: partial scan with reported issues.
int runApplication(std::istream& input, std::ostream& output);
int runApplication(std::istream& input, std::ostream& output, const FileScanner& scanner);
}
