#pragma once

#include "file_metadata.hpp"

#include <map>
#include <string>
#include <vector>

namespace folderflow
{
// An in-memory lookup structure for metadata collected by FileScanner.
class FileIndex
{
public:
    // Adds or replaces a regular-file record. Returns false for non-files or unsafe relative paths.
    bool add(const FileMetadata& file);

    // Keys are normalized relative paths; spelling remains case sensitive.
    // Returned pointers remain valid until this index is destroyed.
    const FileMetadata* findByPath(const fs::path& relativePath) const;
    std::vector<const FileMetadata*> findByExtension(const std::string& extension) const;
    std::size_t size() const { return filesByPath_.size(); }
    // Returns copies of all indexed records in normalized path order.
    std::vector<FileMetadata> allFiles() const;
    bool saveToFile(const fs::path& filePath) const;
    bool loadFromFile(const fs::path& filePath);

private:
    static std::string pathKey(const fs::path& path);
    static std::string normalizedExtension(std::string extension);

    std::map<std::string, FileMetadata> filesByPath_;
    std::map<std::string, std::vector<std::string>> pathsByExtension_;
};
}
