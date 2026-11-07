#include "file_index.hpp"
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace folderflow
{
std::string FileIndex::pathKey(const fs::path& path)
{
    if (path.empty() || path.has_root_path()) return {};
    const auto normalized = path.lexically_normal();
    for (const auto& part : normalized)
        if (part == "..") return {};
    return normalized.generic_u8string();
}

std::string FileIndex::normalizedExtension(std::string extension)
{
    if (!extension.empty() && extension.front() != '.')
        extension.insert(extension.begin(), '.');
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char ch) { return static_cast<char>(ch >= 'A' && ch <= 'Z' ? ch + ('a' - 'A') : ch); });
    return extension;
}

bool FileIndex::add(const FileMetadata& file)
{
    if (file.kind != EntryKind::RegularFile)
        return false;

    const std::string key = pathKey(file.relativePath);
    if (key.empty() || key == ".")
        return false;

    const std::string extension = normalizedExtension(file.extension);
    auto existing = filesByPath_.find(key);
    if (existing != filesByPath_.end())
    {
        const std::string oldExtension = normalizedExtension(existing->second.extension);
        if (oldExtension != extension)
        {
            auto bucket = pathsByExtension_.find(oldExtension);
            if (bucket != pathsByExtension_.end())
            {
                auto& paths = bucket->second;
                paths.erase(std::remove(paths.begin(), paths.end(), key), paths.end());
                if (paths.empty()) pathsByExtension_.erase(bucket);
            }
            pathsByExtension_[extension].push_back(key);
        }
        existing->second = file;
        return true;
    }

    filesByPath_.emplace(key, file);
    pathsByExtension_[extension].push_back(key);
    return true;
}

const FileMetadata* FileIndex::findByPath(const fs::path& relativePath) const
{
    const auto found = filesByPath_.find(pathKey(relativePath));
    return found == filesByPath_.end() ? nullptr : &found->second;
}

std::vector<const FileMetadata*> FileIndex::findByExtension(const std::string& extension) const
{
    std::vector<const FileMetadata*> matches;
    const auto bucket = pathsByExtension_.find(normalizedExtension(extension));
    if (bucket == pathsByExtension_.end()) return matches;

    for (const auto& key : bucket->second)
    {
        const auto file = filesByPath_.find(key);
        if (file != filesByPath_.end()) matches.push_back(&file->second);
    }
        return matches;
}
std::vector<const FileMetadata*> FileIndex::findByName(const std::string& namePart) const
{
    std::vector<const FileMetadata*> matches;

    if (namePart.empty())
        return matches;

    for (const auto& item : filesByPath_)
    {
        const FileMetadata& file = item.second;
        const std::string filename = file.relativePath.filename().generic_u8string();

        if (filename.find(namePart) != std::string::npos)
            matches.push_back(&file);
    }

    return matches;
}

std::vector<FileMetadata> FileIndex::allFiles() const
{
    std::vector<FileMetadata> records;
    records.reserve(filesByPath_.size());

    for (const auto& item : filesByPath_)
    {
        records.push_back(item.second);
    }

    return records;
}

bool FileIndex::saveToFile(const fs::path& filePath) const
{
    std::ofstream output(filePath, std::ios::binary | std::ios::trunc);
    if (!output)
        return false;

    output << "FOLDERFLOW_INDEX 1\n";

    for (const auto& item : filesByPath_)
    {
        const FileMetadata& file = item.second;
        const bool hasSize = file.sizeBytes.has_value();
        const bool hasModifiedTime = file.modifiedTime.has_value();

        output << std::quoted(file.relativePath.generic_u8string()) << ' '
               << std::quoted(file.extension) << ' '
               << hasSize << ' '
               << (hasSize ? *file.sizeBytes : 0) << ' '
               << hasModifiedTime << ' '
               << (hasModifiedTime
                       ? file.modifiedTime->time_since_epoch().count()
                       : 0)
               << '\n';

        if (!output)
            return false;
    }

    output.flush();
    return output.good();
}
bool FileIndex::loadFromFile(const fs::path& filePath)
{
    std::ifstream input(filePath, std::ios::binary);
    std::string header;

    if (!std::getline(input, header) || header != "FOLDERFLOW_INDEX 1")
        return false;

    FileIndex loaded;
    using FileDuration = fs::file_time_type::duration;

    while (input.peek() != std::char_traits<char>::eof())
    {
        std::string path;
        FileMetadata file;
        int hasSize = 0;
        int hasModifiedTime = 0;
        std::uintmax_t size = 0;
        FileDuration::rep ticks = 0;

        if (input.peek() != '"' || !(input >> std::quoted(path)))
            return false;

        input >> std::ws;
        if (input.peek() != '"' ||
            !(input >> std::quoted(file.extension) >> hasSize))
            return false;

        input >> std::ws;
        if (input.peek() == '-' || !(input >> size >> hasModifiedTime >> ticks))
            return false;

        if ((hasSize != 0 && hasSize != 1) ||
            (hasModifiedTime != 0 && hasModifiedTime != 1) ||
            input.get() != '\n')
            return false;

        try
        {
            file.relativePath = fs::u8path(path);
        }
        catch (const fs::filesystem_error&)
        {
            return false;
        }

        file.kind = EntryKind::RegularFile;
        if (hasSize)
            file.sizeBytes = size;
        if (hasModifiedTime)
            file.modifiedTime = fs::file_time_type{FileDuration{ticks}};

        if (loaded.findByPath(file.relativePath) || !loaded.add(file))
            return false;
    }

    if (input.bad() || !input.eof())
        return false;

    filesByPath_.swap(loaded.filesByPath_);
    pathsByExtension_.swap(loaded.pathsByExtension_);
    return true;
}
}
