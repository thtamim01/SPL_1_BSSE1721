#pragma once

#include "file_metadata.hpp"

#include <functional>
#include <system_error>

namespace folderflow
{
// Tests can supply a reader to reproduce a size failure without changing
// production source or racing the operating system.
using FileSizeReader = std::function<std::uintmax_t(const fs::path&, std::error_code&)>;

class FileScanner
{
public:
    explicit FileScanner(FileSizeReader sizeReader = {});
    ScanResult scan(const fs::path& selectedRoot) const;

private:
    FileSizeReader sizeReader_;
};
}
