#pragma once

#include "file_metadata.hpp"

#include <cstdint>
#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

namespace folderflow
{
struct DuplicateGroup
{
    std::vector<fs::path> files;
    std::uintmax_t fileBytes = 0;
};

struct DuplicateReport
{
    std::vector<DuplicateGroup> groups;
    std::vector<std::string> issues;
    std::uintmax_t reclaimableBytes = 0;
};

// The optional transform lets a test force hash collisions. Production uses FNV-1a.
using HashTransform = std::function<std::uint64_t(std::uint64_t)>;
DuplicateReport detectDuplicates(const ScanResult& scan, const HashTransform& transform = {});
int runDuplicateDetection(const fs::path& root, std::ostream& output, std::ostream& errors);
}
