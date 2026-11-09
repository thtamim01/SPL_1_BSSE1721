#pragma once

#include "file_metadata.hpp"

#include <cstddef>
#include <map>

namespace folderflow
{
struct ExtensionUsage
{
    std::uintmax_t files = 0;
    std::uintmax_t bytes = 0;
    std::uintmax_t unknownSizes = 0;
};

struct StorageItem
{
    std::string relativePath;
    std::uintmax_t bytes;
};

struct StorageReport
{
    std::uintmax_t files = 0;
    std::uintmax_t bytes = 0;
    std::uintmax_t unknownSizes = 0;
    std::map<std::string, ExtensionUsage> byExtension;
    std::vector<StorageItem> largestFiles;
    std::vector<StorageItem> largestFolders;
};

// Uses scan metadata only. No result means the byte total would overflow.
// Empty extension keys represent files without an extension.
std::optional<StorageReport> analyzeStorage(const ScanResult& scan, std::size_t limit);
long double storagePercent(std::uintmax_t bytes, std::uintmax_t totalBytes);
std::string formatStorageSize(std::uintmax_t bytes);
}
