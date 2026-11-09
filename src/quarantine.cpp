#include "quarantine.hpp"

#include <filesystem>
#include <exception>
#include <fstream>
#include <iomanip>
#include <ios>
#include <limits>
#include <ostream>
#include <random>
#include <sstream>
#include <string>
#include <system_error>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__linux__)
#include <cerrno>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace folderflow
{
namespace
{
namespace fs = std::filesystem;
const fs::path quarantineName = ".folderflow-quarantine";

bool plain(const fs::path& path, bool directory)
{
    std::error_code ec;
    const auto status = fs::symlink_status(path, ec);
    if (ec || fs::is_symlink(status) ||
        (directory ? !fs::is_directory(status) : !fs::is_regular_file(status)))
        return false;
#ifdef _WIN32
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES ||
        (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
        return false;
#endif
    return true;
}

bool plainChain(const fs::path& path)
{
    auto current = path.root_path();
    if (!plain(current, true))
        return false;
    for (const auto& component : path.relative_path())
    {
        current /= component;
        if (!plain(current, true))
            return false;
    }
    return true;
}

bool validRelative(const fs::path& relative)
{
    if (relative.empty() || relative.is_absolute() || relative.has_root_path())
        return false;
    for (const auto& component : relative)
        if (component.empty() || component == "." || component == ".." ||
            component == ".git" || component == quarantineName ||
            component == ".folderflow-undo")
            return false;
    return true;
}

bool validId(const std::string& id)
{
    if (id.size() != 32)
        return false;
    for (const char c : id)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return false;
    return true;
}

std::string newId()
{
    static constexpr char digits[] = "0123456789abcdef";
    std::random_device random;
    std::string id;
    for (int i = 0; i < 16; ++i)
    {
        const auto byte = static_cast<unsigned int>(random() & 0xffu);
        id += digits[byte >> 4];
        id += digits[byte & 15u];
    }
    return id;
}

bool syncRecord(const fs::path& record)
{
#ifdef _WIN32
    HANDLE handle = CreateFileW(record.c_str(), GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        return false;
    const DWORD attributes = GetFileAttributesW(record.c_str());
    const bool success = attributes != INVALID_FILE_ATTRIBUTES &&
                         !(attributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
                         FlushFileBuffers(handle);
    CloseHandle(handle);
    return success;
#elif defined(__linux__)
    const int fd = open(record.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0)
        return false;
    const bool success = fsync(fd) == 0;
    close(fd);
    return success;
#else
    (void)record;
    return false;
#endif
}

bool syncDirectory(const fs::path& directory)
{
#ifdef __linux__
    const int fd = open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0)
        return false;
    const bool success = fsync(fd) == 0;
    close(fd);
    return success;
#else
    // Portable C++17 and this Windows API do not provide a portable directory flush.
    (void)directory;
    return true;
#endif
}

bool append(const fs::path& record, const std::string& text)
{
    std::ofstream file(record, std::ios::binary | std::ios::app);
    if (!file)
        return false;
    file << text;
    file.flush();
    const bool written = static_cast<bool>(file);
    file.close();
    return written && static_cast<bool>(file) && syncRecord(record);
}

bool moveWithoutOverwrite(const fs::path& source, const fs::path& target)
{
#ifdef _WIN32
    return MoveFileExW(source.c_str(), target.c_str(), 0) != 0;
#elif defined(__linux__)
    return syscall(SYS_renameat2, AT_FDCWD, source.c_str(),
                   AT_FDCWD, target.c_str(), RENAME_NOREPLACE) == 0;
#else
    (void)source;
    (void)target;
    return false;
#endif
}

struct Record
{
    fs::path original;
    fs::path stored;
    std::uintmax_t bytes = 0;
    std::string last;
};

bool readRecord(const fs::path& path, const std::string& id, Record& result)
{
    if (!plain(path, false))
        return false;
    std::ifstream file(path, std::ios::binary);
    std::string header, original, stored, number, tag;
    if (!std::getline(file, header) || header != "FOLDERFLOW_QUARANTINE_V1" ||
        !(file >> std::quoted(original) >> std::quoted(stored) >> result.bytes) ||
        !std::getline(file, number) || !number.empty() ||
        !std::getline(file, tag) || tag != "PENDING")
        return false;
    try
    {
        result.original = fs::u8path(original);
        result.stored = fs::u8path(stored);
    }
    catch (const std::exception&)
    {
        return false;
    }
    if (!validRelative(result.original) ||
        result.stored != quarantineName / id / "payload")
        return false;
    result.last = "PENDING";
    while (std::getline(file, tag))
    {
        if ((result.last == "PENDING" && tag == "STORED") ||
            ((result.last == "PENDING" || result.last == "STORED") && tag == "RESTORE_PENDING") ||
            (result.last == "RESTORE_PENDING" && tag == "RESTORED"))
            result.last = tag;
        else
            return false;
    }
    return file.eof() && !file.bad();
}

bool rootPath(const fs::path& supplied, fs::path& root)
{
    std::error_code ec;
    root = fs::absolute(supplied, ec).lexically_normal();
    return !supplied.empty() && !ec && plainChain(root);
}

bool storedLocation(const fs::path& root, const std::string& id,
                    Record& record, bool& payload)
{
    const auto directory = root / quarantineName;
    if (!plainChain(directory) || !validId(id) ||
        !plain(directory / id, true) ||
        !readRecord(directory / id / "record.txt", id, record))
        return false;
    std::error_code ec;
    const auto path = root / record.stored;
    const bool exists = fs::exists(fs::symlink_status(path, ec));
    if (ec && ec != std::errc::no_such_file_or_directory)
        return false;
    payload = exists;
    return !payload || (plain(path, false) && fs::file_size(path, ec) == record.bytes && !ec);
}
}

int quarantineFile(const fs::path& selected, const fs::path& relative,
                   std::ostream& output, std::ostream& errors)
{
    fs::path root;
    if (!rootPath(selected, root) || !validRelative(relative) ||
        !plainChain((root / relative).parent_path()) ||
        !plain(root / relative, false))
    {
        errors << "Unsafe or missing selected file.\n";
        return 1;
    }
    const auto source = root / relative;
    std::error_code ec;
    const auto bytes = fs::file_size(source, ec);
    if (ec)
    {
        errors << "Cannot inspect selected file.\n";
        return 1;
    }

    const auto directory = root / quarantineName;
    const bool createdRoot = fs::create_directory(directory, ec);
    if (!createdRoot)
    {
        if (ec || !plain(directory, true))
        {
            errors << "Cannot use quarantine directory.\n";
            return 1;
        }
    }
    if (!plainChain(directory) || (createdRoot && !syncDirectory(root)))
    {
        errors << "Unsafe quarantine directory.\n";
        return 1;
    }

    std::string id;
    fs::path entry;
    bool created = false;
    for (int attempts = 0; attempts < 20 && !created; ++attempts)
    {
        id = newId();
        entry = directory / id;
        ec.clear();
        created = fs::create_directory(entry, ec);
        if (ec && ec != std::errc::file_exists)
        {
            errors << "Cannot reserve recovery identifier.\n";
            return 1;
        }
    }
    if (!created)
    {
        errors << "Cannot reserve recovery identifier.\n";
        return 1;
    }
    const auto stored = quarantineName / id / "payload";
    const auto record = entry / "record.txt";
    std::ostringstream metadata;
    metadata << "FOLDERFLOW_QUARANTINE_V1\n"
             << std::quoted(relative.generic_u8string()) << '\n'
             << std::quoted(stored.generic_u8string()) << '\n'
             << bytes << "\nPENDING\n";
    if (!syncDirectory(directory) || !append(record, metadata.str()) ||
        !syncDirectory(entry))
    {
        errors << "Recovery record could not be saved; file was not moved. ID: " << id << '\n';
        return 1;
    }
    if (!plainChain(source.parent_path()) || !plain(source, false) ||
        fs::file_size(source, ec) != bytes || ec ||
        !moveWithoutOverwrite(source, root / stored))
    {
        errors << "Move failed or selected file changed; recovery ID: " << id << '\n';
        return 1;
    }
    if (!syncDirectory(entry) || !syncDirectory(source.parent_path()))
    {
        errors << "File moved, but directory durability is unverified. Recovery ID: " << id << '\n';
        return 2;
    }
    if (!append(record, "STORED\n"))
    {
        errors << "File moved, but recovery record is incomplete. Recovery ID: " << id << '\n';
        return 2;
    }
    output << "QUARANTINED " << relative.generic_u8string() << " ID: " << id << '\n';
    return 0;
}

int listQuarantine(const fs::path& selected, std::ostream& output, std::ostream& errors)
{
    fs::path root;
    if (!rootPath(selected, root))
    {
        errors << "Invalid folder path.\n";
        return 1;
    }
    const auto directory = root / quarantineName;
    std::error_code ec;
    if (!fs::exists(directory, ec) && !ec)
    {
        output << "No quarantine records.\n";
        return 0;
    }
    if (!plainChain(directory))
    {
        errors << "Unsafe quarantine directory.\n";
        return 1;
    }
    int status = 0;
    fs::directory_iterator iterator(directory, ec);
    const fs::directory_iterator end;
    if (ec)
    {
        errors << "Cannot list quarantine records.\n";
        return 2;
    }
    while (iterator != end)
    {
        const auto id = iterator->path().filename().u8string();
        Record record;
        bool payload = false;
        if (!storedLocation(root, id, record, payload))
        {
            errors << "UNVERIFIED quarantine record: " << id << '\n';
            status = 2;
        }
        else
        {
            const bool complete = (payload && record.last == "STORED") ||
                                  (!payload && record.last == "RESTORED");
            const char* state = complete ? (payload ? "QUARANTINED" : "RESTORED") :
                                record.last == "RESTORE_PENDING" ? "RESTORE_PENDING" :
                                record.last == "PENDING" ? "PENDING" : "UNVERIFIED";
            if (!complete)
            {
                errors << "Incomplete or inconsistent recovery record: " << id << '\n';
                status = 2;
            }
            output << id << " " << state << " " << record.original.generic_u8string()
                   << " -> " << record.stored.generic_u8string() << '\n';
        }
        iterator.increment(ec);
        if (ec)
        {
            errors << "Cannot finish listing quarantine records.\n";
            return 2;
        }
    }
    return status;
}

int restoreQuarantine(const fs::path& selected, const std::string& id,
                      std::ostream& output, std::ostream& errors)
{
    fs::path root;
    Record record;
    bool payload = false;
    if (!rootPath(selected, root) || !storedLocation(root, id, record, payload))
    {
        errors << "Missing or damaged recovery record.\n";
        return 1;
    }
    if (!payload || record.last == "RESTORED")
    {
        errors << "Quarantined file is unavailable or already restored.\n";
        return 1;
    }
    const auto destination = root / record.original;
    if (!plainChain(destination.parent_path()))
    {
        errors << "Original folder is missing or unsafe.\n";
        return 1;
    }
    std::error_code ec;
    const auto state = fs::symlink_status(destination, ec);
    if (ec && ec != std::errc::no_such_file_or_directory)
    {
        errors << "Cannot inspect original destination.\n";
        return 1;
    }
    if (fs::exists(state))
    {
        errors << "Original destination already exists; restore refused.\n";
        return 1;
    }
    const auto journal = root / quarantineName / id / "record.txt";
    if (record.last != "RESTORE_PENDING" && !append(journal, "RESTORE_PENDING\n"))
    {
        errors << "Cannot record restore intent; file was not moved.\n";
        return 1;
    }
    if (!plain(root / record.stored, false) ||
        !moveWithoutOverwrite(root / record.stored, destination))
    {
        errors << "Restore move failed; recovery ID: " << id << '\n';
        return 1;
    }
    if (!syncDirectory(destination.parent_path()) ||
        !syncDirectory((root / record.stored).parent_path()))
    {
        errors << "File restored, but directory durability is unverified. Recovery ID: " << id << '\n';
        return 2;
    }
    if (!append(journal, "RESTORED\n"))
    {
        errors << "File restored, but journal completion failed. Recovery ID: " << id << '\n';
        return 2;
    }
    output << "RESTORED " << record.original.generic_u8string() << " ID: " << id << '\n';
    return 0;
}
}
