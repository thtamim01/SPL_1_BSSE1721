#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace folderflow
{
namespace fs = std::filesystem;

enum class EntryKind { RegularFile, Directory, SkippedLink, SkippedOther };

struct FileMetadata
{
    fs::path relativePath;
    EntryKind kind = EntryKind::SkippedOther;
    std::string extension;
    // Missing metadata is different from a successfully measured zero.
    std::optional<std::uintmax_t> sizeBytes;
    std::optional<fs::file_time_type> modifiedTime;
};

struct ScanIssue
{
    fs::path relativePath;
    std::string operation;
    std::string message;
};

struct ScanResult
{
    fs::path root;
    bool rootValid = false;
    std::vector<FileMetadata> entries;
    std::vector<ScanIssue> issues;
    std::uintmax_t files = 0;
    std::uintmax_t folders = 0;
    std::uintmax_t totalBytes = 0;
    std::uintmax_t excludedEntries = 0;
    std::uintmax_t skippedLinks = 0;
    std::uintmax_t skippedOther = 0;

    bool complete() const { return rootValid && issues.empty(); }
};
}
