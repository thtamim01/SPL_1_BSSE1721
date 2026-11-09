#include "duplicate_detector.hpp"

#include "scanner.hpp"

#include <array>
#include <fstream>
#include <limits>
#include <map>
#include <ostream>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace folderflow
{
namespace
{
constexpr std::size_t bufferSize = 64 * 1024;

bool safePath(const fs::path& root, const fs::path& relative)
{
    if (relative.empty() || relative.is_absolute())
        return false;
    fs::path current = root;
    std::error_code ec;
    for (const auto& part : relative)
    {
        if (part == "." || part == "..")
            return false;
        current /= part;
        const auto status = fs::symlink_status(current, ec);
        if (ec || fs::is_symlink(status))
            return false;
#ifdef _WIN32
        const DWORD attributes = GetFileAttributesW(current.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES ||
            (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
            return false;
#endif
    }
    return true;
}

bool stableFile(const ScanResult& scan, const FileMetadata& entry)
{
    if (!entry.sizeBytes || !entry.modifiedTime ||
        !safePath(scan.root, entry.relativePath))
        return false;
    std::error_code ec;
    const auto path = scan.root / entry.relativePath;
    const auto status = fs::symlink_status(path, ec);
    if (ec || !fs::is_regular_file(status))
        return false;
    const auto bytes = fs::file_size(path, ec);
    if (ec || bytes != *entry.sizeBytes)
        return false;
    const auto time = fs::last_write_time(path, ec);
    return !ec && time == *entry.modifiedTime;
}

bool hashFile(const ScanResult& scan, const FileMetadata& entry, std::uint64_t& hash)
{
    if (!stableFile(scan, entry))
        return false;
    std::ifstream stream(scan.root / entry.relativePath, std::ios::binary);
    if (!stream)
        return false;
    std::array<char, bufferSize> buffer{};
    std::uintmax_t count = 0;
    hash = 14695981039346656037ULL;
    while (stream)
    {
        stream.read(buffer.data(), buffer.size());
        const auto read = stream.gcount();
        if (stream.bad() || (stream.fail() && !stream.eof()))
            return false;
        count += static_cast<std::uintmax_t>(read);
        for (std::streamsize i = 0; i < read; ++i)
        {
            hash ^= static_cast<unsigned char>(buffer[static_cast<std::size_t>(i)]);
            hash *= 1099511628211ULL;
        }
    }
    return count == *entry.sizeBytes && stableFile(scan, entry);
}

enum class Comparison { Same, Different, Unverifiable };

Comparison compareFiles(const ScanResult& scan, const FileMetadata& first,
                        const FileMetadata& second)
{
    if (!stableFile(scan, first) || !stableFile(scan, second))
        return Comparison::Unverifiable;
    std::ifstream left(scan.root / first.relativePath, std::ios::binary);
    std::ifstream right(scan.root / second.relativePath, std::ios::binary);
    if (!left || !right)
        return Comparison::Unverifiable;

    std::array<char, bufferSize> leftBuffer{};
    std::array<char, bufferSize> rightBuffer{};
    std::uintmax_t total = 0;
    bool different = false;
    while (left && right)
    {
        left.read(leftBuffer.data(), leftBuffer.size());
        right.read(rightBuffer.data(), rightBuffer.size());
        const auto a = left.gcount();
        const auto b = right.gcount();
        if (left.bad() || right.bad() ||
            (left.fail() && !left.eof()) || (right.fail() && !right.eof()))
            return Comparison::Unverifiable;
        total += static_cast<std::uintmax_t>(a);
        if (a != b)
            different = true;
        for (std::streamsize i = 0; i < a && i < b; ++i)
            if (leftBuffer[static_cast<std::size_t>(i)] !=
                rightBuffer[static_cast<std::size_t>(i)])
                different = true;
    }
    if (!stableFile(scan, first) || !stableFile(scan, second) ||
        total != *first.sizeBytes)
        return Comparison::Unverifiable;
    return different ? Comparison::Different : Comparison::Same;
}

void issue(DuplicateReport& report, const fs::path& path, const char* reason)
{
    report.issues.push_back(path.generic_u8string() + ": " + reason);
}
}

DuplicateReport detectDuplicates(const ScanResult& scan, const HashTransform& transform)
{
    DuplicateReport report;
    for (const auto& scanIssue : scan.issues)
        issue(report, scanIssue.relativePath, scanIssue.message.c_str());
    if (!scan.rootValid)
    {
        report.issues.push_back("Invalid folder path.");
        return report;
    }
    std::map<std::uintmax_t, std::vector<const FileMetadata*>> bySize;
    for (const auto& entry : scan.entries)
        if (entry.kind == EntryKind::RegularFile)
        {
            if (entry.sizeBytes)
                bySize[*entry.sizeBytes].push_back(&entry);
            else
                issue(report, entry.relativePath, "size unavailable");
        }

    for (const auto& sizeBucket : bySize)
    {
        if (sizeBucket.second.size() < 2)
            continue;
        std::map<std::uint64_t, std::vector<const FileMetadata*>> byHash;
        for (const auto* entry : sizeBucket.second)
        {
            std::uint64_t hash = 0;
            if (!hashFile(scan, *entry, hash))
            {
                issue(report, entry->relativePath, "cannot verify content or file changed");
                continue;
            }
            byHash[transform ? transform(hash) : hash].push_back(entry);
        }
        for (const auto& hashBucket : byHash)
        {
            std::vector<std::vector<const FileMetadata*>> confirmed;
            for (const auto* candidate : hashBucket.second)
            {
                bool placed = false;
                bool unverifiable = false;
                for (auto& group : confirmed)
                {
                    const auto comparison = compareFiles(scan, *group.front(), *candidate);
                    if (comparison == Comparison::Unverifiable)
                    {
                        unverifiable = true;
                        issue(report, candidate->relativePath, "comparison could not be verified");
                        break;
                    }
                    if (comparison == Comparison::Same)
                    {
                        group.push_back(candidate);
                        placed = true;
                        break;
                    }
                }
                if (!placed && !unverifiable)
                    confirmed.push_back({candidate});
            }
            for (const auto& group : confirmed)
            {
                if (group.size() < 2)
                    continue;
                bool valid = true;
                for (const auto* entry : group)
                    if (!stableFile(scan, *entry))
                    {
                        valid = false;
                        issue(report, entry->relativePath, "file changed before reporting");
                    }
                const auto extra = static_cast<std::uintmax_t>(group.size() - 1);
                if (!valid || (sizeBucket.first != 0 &&
                    extra > (std::numeric_limits<std::uintmax_t>::max() -
                             report.reclaimableBytes) / sizeBucket.first))
                {
                    if (valid)
                        report.issues.push_back("Potential reclaimable byte count overflow.");
                    continue;
                }
                DuplicateGroup duplicate;
                duplicate.fileBytes = sizeBucket.first;
                for (const auto* entry : group)
                    duplicate.files.push_back(entry->relativePath);
                report.reclaimableBytes += extra * sizeBucket.first;
                report.groups.push_back(duplicate);
            }
        }
    }
    return report;
}

int runDuplicateDetection(const fs::path& root, std::ostream& output, std::ostream& errors)
{
    const auto scan = FileScanner{}.scan(root);
    if (!scan.rootValid)
    {
        errors << "Invalid folder path.\n";
        return 1;
    }
    const auto report = detectDuplicates(scan);
    for (const auto& group : report.groups)
    {
        output << "Duplicate group (" << group.fileBytes << " bytes each):\n";
        for (const auto& path : group.files)
            output << "  - " << path.generic_u8string() << '\n';
    }
    output << "Confirmed groups: " << report.groups.size() << '\n'
           << "Potential reclaimable logical bytes: " << report.reclaimableBytes << '\n';
    for (const auto& message : report.issues)
        errors << "UNVERIFIED: " << message << '\n';
    return report.issues.empty() ? 0 : 2;
}
}
