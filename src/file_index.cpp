#include "file_index.hpp"

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
}
