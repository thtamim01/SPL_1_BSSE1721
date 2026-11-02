#include "scanner.hpp"

#include <algorithm>
#include <limits>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace folderflow
{
namespace
{
bool isLinkOrReparsePoint(const fs::path& path, fs::file_status status,
                          std::error_code& error)
{
    error.clear();
    if (fs::is_symlink(status)) { return true; }
#ifdef _WIN32
    // Junctions and other reparse points can redirect Windows traversal.
    // Conservatively skip all reparse points, including cloud placeholders.
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        error = std::error_code(static_cast<int>(GetLastError()), std::system_category());
        return false;
    }
    return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
#else
    (void)path;
    return false;
#endif
}
}

FileScanner::FileScanner(FileSizeReader sizeReader) : sizeReader_(std::move(sizeReader))
{
    if (!sizeReader_)
    {
        sizeReader_ = [](const fs::path& path, std::error_code& error)
        {
            return fs::file_size(path, error);
        };
    }
}

ScanResult FileScanner::scan(const fs::path& selectedRoot) const
{
    ScanResult result;
    auto report = [&](const fs::path& relative, const std::string& operation,
                      const std::string& message)
    {
        result.issues.push_back({relative, operation, message});
    };
    if (selectedRoot.empty())
    {
        report(".", "root", "No directory path was supplied.");
        return result;
    }

    std::error_code error;
    result.root = fs::absolute(selectedRoot, error).lexically_normal();
    if (error)
    {
        report(".", "root", error.message());
        return result;
    }
    if (result.root != result.root.root_path() && result.root.filename().empty())
    {
        result.root = result.root.parent_path();
    }
    for (const auto& component : result.root)
    {
        if (component == ".git")
        {
            report(".", "root", "Directories inside .git are excluded.");
            return result;
        }
    }
    const auto rootStatus = fs::symlink_status(result.root, error);
    if (error || !fs::exists(rootStatus))
    {
        report(".", "root", error ? error.message() : "Directory does not exist.");
        return result;
    }
    const bool rootIsLink = isLinkOrReparsePoint(result.root, rootStatus, error);
    if (error || rootIsLink || !fs::is_directory(rootStatus) || result.root.filename() == ".git")
    {
        report(".", "root", error ? error.message() :
               "Select an ordinary directory outside .git; link/reparse roots are not scanned.");
        return result;
    }
    result.rootValid = true;

    // Iterative depth-first traversal. The vector acts as a stack of folders.
    // Opening folders individually lets a denied child be reported while other
    // pending folders can still be scanned.
    std::vector<fs::path> pending{result.root};
    while (!pending.empty())
    {
        const auto directory = pending.back();
        pending.pop_back();
        fs::directory_iterator iterator(directory, fs::directory_options::none, error);
        const fs::directory_iterator end;
        if (error)
        {
            report(directory.lexically_relative(result.root), "open directory", error.message());
            continue;
        }

        auto visit = [&](const fs::path& path)
        {
            const auto relative = path.lexically_relative(result.root);
            if (path.filename() == ".git")
            {
                ++result.excludedEntries;
                return;
            }
            const auto status = fs::symlink_status(path, error);
            if (error || !fs::exists(status))
            {
                report(relative, "read entry type", error ? error.message() : "Entry disappeared.");
                return;
            }
            const bool isLink = isLinkOrReparsePoint(path, status, error);
            if (error)
            {
                report(relative, "read link attributes", error.message());
                return;
            }

            FileMetadata record;
            record.relativePath = relative;
            if (isLink)
            {
                record.kind = EntryKind::SkippedLink;
                ++result.skippedLinks;
            }
            else if (fs::is_directory(status))
            {
                record.kind = EntryKind::Directory;
                ++result.folders;
                pending.push_back(path);
            }
            else if (fs::is_regular_file(status))
            {
                record.kind = EntryKind::RegularFile;
                record.extension = path.extension().u8string();
                ++result.files;
                error.clear();
                const auto bytes = sizeReader_(path, error);
                if (error)
                {
                    report(relative, "read file size", error.message());
                }
                else
                {
                    record.sizeBytes = bytes;
                    if (bytes > std::numeric_limits<std::uintmax_t>::max() - result.totalBytes)
                    {
                        report(relative, "sum file sizes", "Total byte count would overflow.");
                    }
                    else { result.totalBytes += bytes; }
                }
            }
            else
            {
                record.kind = EntryKind::SkippedOther;
                ++result.skippedOther;
            }
            if (record.kind == EntryKind::Directory || record.kind == EntryKind::RegularFile)
            {
                error.clear();
                const auto time = fs::last_write_time(path, error);
                if (error) { report(relative, "read modification time", error.message()); }
                else { record.modifiedTime = time; }
            }
            result.entries.push_back(std::move(record));
        };

        while (iterator != end)
        {
            visit(iterator->path());
            iterator.increment(error);
            if (error)
            {
                report(directory.lexically_relative(result.root), "advance directory", error.message());
                break;
            }
        }
    }

    // Filesystem iteration order is unspecified; sorting makes output stable.
    std::sort(result.entries.begin(), result.entries.end(),
              [](const FileMetadata& left, const FileMetadata& right)
              {
                  return left.relativePath.generic_u8string() < right.relativePath.generic_u8string();
              });
    return result;
}
}
