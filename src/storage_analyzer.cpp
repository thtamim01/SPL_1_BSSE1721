#include "storage_analyzer.hpp"

#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

namespace folderflow
{
namespace
{
bool ranksHigher(const StorageItem& left, const StorageItem& right)
{
    if (left.bytes != right.bytes)
        return left.bytes > right.bytes;
    return left.relativePath < right.relativePath;
}

// The weakest retained item stays at the root, where it can be replaced.
class TopItems
{
public:
    explicit TopItems(std::size_t limit) : limit_(limit) {}

    void consider(StorageItem item)
    {
        if (limit_ == 0)
            return;

        if (items_.size() == limit_)
        {
            if (ranksHigher(item, items_.front()))
            {
                items_.front() = std::move(item);
                siftDown();
            }
            return;
        }

        items_.push_back(std::move(item));
        std::size_t child = items_.size() - 1;
        while (child > 0)
        {
            const std::size_t parent = (child - 1) / 2;
            if (!ranksHigher(items_[parent], items_[child]))
                break;
            std::swap(items_[parent], items_[child]);
            child = parent;
        }
    }

    std::vector<StorageItem> takeRanked()
    {
        std::vector<StorageItem> ranked(items_.size());
        while (!items_.empty())
        {
            ranked[items_.size() - 1] = std::move(items_.front());
            if (items_.size() > 1)
                items_.front() = std::move(items_.back());
            items_.pop_back();
            siftDown();
        }
        return ranked;
    }

private:
    void siftDown()
    {
        std::size_t parent = 0;
        while (parent < items_.size() / 2)
        {
            std::size_t weakerChild = 2 * parent + 1;
            const std::size_t right = weakerChild + 1;
            if (right < items_.size() && ranksHigher(items_[weakerChild], items_[right]))
                weakerChild = right;

            if (!ranksHigher(items_[parent], items_[weakerChild]))
                break;
            std::swap(items_[parent], items_[weakerChild]);
            parent = weakerChild;
        }
    }

    std::size_t limit_;
    std::vector<StorageItem> items_;
};
}

std::optional<StorageReport> analyzeStorage(const ScanResult& scan, std::size_t limit)
{
    StorageReport report;
    TopItems files(limit);
    TopItems folders(limit);
    std::map<std::string, std::uintmax_t> folderBytes;

    for (const auto& entry : scan.entries)
    {
        if (entry.kind == EntryKind::Directory)
            folderBytes.try_emplace(entry.relativePath.generic_u8string(), 0);
        if (entry.kind != EntryKind::RegularFile)
            continue;

        std::string extension = entry.extension;
        for (char& ch : extension)
        {
            if (ch >= 'A' && ch <= 'Z')
                ch = static_cast<char>(ch - 'A' + 'a');
        }
        auto& group = report.byExtension[extension];
        ++report.files;
        ++group.files;

        if (!entry.sizeBytes)
        {
            ++report.unknownSizes;
            ++group.unknownSizes;
            continue;
        }

        const auto bytes = *entry.sizeBytes;
        if (bytes > std::numeric_limits<std::uintmax_t>::max() - report.bytes)
            return std::nullopt;

        report.bytes += bytes;
        group.bytes += bytes;
        files.consider({entry.relativePath.generic_u8string(), bytes});

        // Each parent includes descendants. The selected root is shown separately.
        for (auto parent = entry.relativePath.parent_path(); parent.has_relative_path();
             parent = parent.parent_path())
        {
            folderBytes[parent.generic_u8string()] += bytes;
        }
    }

    for (const auto& folder : folderBytes)
        folders.consider({folder.first, folder.second});

    report.largestFiles = files.takeRanked();
    report.largestFolders = folders.takeRanked();
    return report;
}

long double storagePercent(std::uintmax_t bytes, std::uintmax_t totalBytes)
{
    if (totalBytes == 0)
        return 0.0L;
    return 100.0L * static_cast<long double>(bytes) / totalBytes;
}

std::string formatStorageSize(std::uintmax_t bytes)
{
    if (bytes < 1000)
        return std::to_string(bytes) + " B";

    const char* units[] = {"KB", "MB", "GB", "TB", "PB", "EB"};
    std::size_t unit = 0;
    auto magnitude = bytes / 1000;
    long double scaled = static_cast<long double>(bytes) / 1000;
    while (magnitude >= 1000 && unit + 1 < sizeof(units) / sizeof(units[0]))
    {
        magnitude /= 1000;
        scaled /= 1000;
        ++unit;
    }

    std::ostringstream output;
    output << std::fixed << std::setprecision(2) << scaled << ' ' << units[unit];
    return output.str();
}
}
