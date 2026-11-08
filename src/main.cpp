#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "rule_config.hpp"
#include "file_index.hpp"
#include "rule_config.hpp"
#include "application.hpp"
#include "file_index.hpp"
#include "scanner.hpp"
#include <cctype>
#include <iostream>
#include <string>
#include <map>
#include <vector>

int main(int argc, char* argv[])
{
    using namespace folderflow;

    if (argc == 1)
        return runApplication(std::cin, std::cout);

    const std::string command = argv[1];
    FileIndex index;
    if ((command == "--evaluate-rules" || command == "--dry-run" || command == "--organize") &&
    argc == 4)
    {
        const auto config = loadExtensionRules(fs::u8path(argv[3]));
        if (!config.error.empty())
        {
            std::cerr << config.error << '\n';
            return 1;
        }

        const auto scan = FileScanner{}.scan(fs::u8path(argv[2]));
        if (!scan.rootValid)
        {
            std::cerr << "Invalid folder path.\n";
            return 1;
        }
        if (!scan.complete())
        {
            std::cerr << "Scan incomplete. Rule evaluation stopped.\n";
            return 2;
        }

        struct DestinationGroup
        {
            std::string displayPath;
            std::vector<std::string> sources;
        };

        std::map<std::string, DestinationGroup> destinations;

        for (const auto& entry : scan.entries)
        {
            if (entry.kind != EntryKind::RegularFile)
                continue;

            std::string extension = entry.extension;
            for (char& ch : extension)
            {
                ch = static_cast<char>(
                    std::tolower(static_cast<unsigned char>(ch)));
            }

            const ExtensionRule* match = nullptr;
            for (const auto& rule : config.rules)
            {
                if (rule.extension == extension)
                {
                    match = &rule;
                    break;
                }
            }

            std::cout << entry.relativePath.generic_u8string();

            if (match)
            {
                const auto destination =
                    match->destination / entry.relativePath.filename();
                const std::string destinationPath =
                    destination.generic_u8string();

                std::string destinationKey = destinationPath;
                for (char& ch : destinationKey)
                {
                    if (ch >= 'A' && ch <= 'Z')
                        ch = static_cast<char>(ch - 'A' + 'a');
                }

                auto& group = destinations[destinationKey];
                if (group.sources.empty())
                    group.displayPath = destinationPath;

                group.sources.push_back(
                    entry.relativePath.generic_u8string());

#ifdef _WIN32
                if (command == "--dry-run")
                {
                    const auto parent = (scan.root / destination).parent_path();
                    const DWORD attributes = GetFileAttributesW(parent.c_str());
                    if (attributes != INVALID_FILE_ATTRIBUTES &&
                        (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
                    {
                        std::cout << " -> " << destinationPath
                                  << " [SKIP: destination folder is a junction or other reparse point]\n";
                        continue;
                    }
                }
#endif
                std::cout << " -> " << destinationPath;

                if (command == "--dry-run")
                {
                    std::error_code ec;
                    const bool occupied = fs::exists(scan.root / destination, ec);

                    if (ec)
                        std::cout << " [SKIP: cannot check destination]";
                    else if (occupied)
                        std::cout << " [SKIP: destination already exists]";
                    else
                        std::cout << " [DRY RUN: move proposed]";
                }

                std::cout << '\n';
            }
            else
            {
                std::cout << " -> no matching rule\n";
            }
        }

        for (const auto& [key, group] : destinations)
        {
            if (group.sources.size() < 2)
                continue;

            std::cout << "Conflict: " << group.displayPath << '\n';
            for (const auto& source : group.sources)
                std::cout << "  - " << source << '\n';
        }

        if (command == "--organize")
        {
#ifdef _WIN32
            std::error_code ec;
            const auto root = fs::absolute(scan.root, ec).lexically_normal();
            if (ec)
            {
                std::cerr << "Cannot resolve the selected folder.\n";
                return 1;
            }

            const auto plainDirectoryChain = [](fs::path directory)
            {
                while (!directory.empty())
                {
                    const DWORD attributes = GetFileAttributesW(directory.c_str());
                    if (attributes == INVALID_FILE_ATTRIBUTES ||
                        !(attributes & FILE_ATTRIBUTE_DIRECTORY) ||
                        (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
                        return false;

                    const auto parent = directory.parent_path();
                    if (parent == directory)
                        break;
                    directory = parent;
                }
                return true;
            };

            if (!plainDirectoryChain(root))
            {
                std::cerr << "Selected folder has an inaccessible or linked parent.\n";
                return 1;
            }

            bool failed = false;
            for (const auto& item : destinations)
            {
                const auto& group = item.second;
                if (group.sources.size() != 1)
                    continue; // The existing conflict report already lists these files.

                const auto relativeSource = fs::u8path(group.sources.front());
                const auto relativeTarget = fs::u8path(group.displayPath);
                const auto folder = relativeTarget.parent_path();
                const auto folderName = folder.native();

                if (folder.empty() || folder != folder.filename() ||
                    folder == "." || folder == ".." ||
                    folderName.find_first_of(L"<>:\"/\\|?*") != std::wstring::npos ||
                    folderName.back() == L'.' || folderName.back() == L' ')
                {
                    std::cout << "SKIP: invalid destination folder for "
                              << group.displayPath << '\n';
                    continue;
                }

                const auto source = root / relativeSource;
                const auto target = root / relativeTarget;
                const auto targetFolder = target.parent_path();
                const DWORD sourceAttributes = GetFileAttributesW(source.c_str());

                if (!plainDirectoryChain(source.parent_path()) ||
                    sourceAttributes == INVALID_FILE_ATTRIBUTES ||
                    (sourceAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
                {
                    std::cout << "SKIP: unsafe source " << group.sources.front() << '\n';
                    continue;
                }

                DWORD folderAttributes = GetFileAttributesW(targetFolder.c_str());
                if (folderAttributes == INVALID_FILE_ATTRIBUTES)
                {
                    const DWORD error = GetLastError();
                    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
                    {
                        fs::create_directory(targetFolder, ec);
                        if (ec)
                        {
                            std::cerr << "FAILED: cannot create " << folder.generic_u8string()
                                      << ": " << ec.message() << '\n';
                            failed = true;
                            continue;
                        }
                    }
                }

                if (!plainDirectoryChain(targetFolder))
                {
                    std::cout << "SKIP: unsafe destination folder for " << group.displayPath << '\n';
                    continue;
                }

                if (GetFileAttributesW(target.c_str()) != INVALID_FILE_ATTRIBUTES)
                {
                    std::cout << "SKIP: destination already exists " << group.displayPath << '\n';
                    continue;
                }

                if (!MoveFileExW(source.c_str(), target.c_str(), 0))
                {
                    const DWORD error = GetLastError();
                    std::cerr << "FAILED: " << group.sources.front()
                              << " (Windows error " << error << ")\n";
                    failed = true;
                    continue;
                }

                std::cout << "MOVED: " << group.sources.front()
                          << " -> " << group.displayPath << '\n';
            }

            return failed ? 2 : 0;
#else
            std::cerr << "--organize is currently supported on Windows only.\n";
            return 1;
#endif
        }


        return 0;
    }
    if (command == "--check-rules" && argc == 3)
    {
         const auto result =
            folderflow::loadExtensionRules(std::filesystem::u8path(argv[2]));

        if (!result.error.empty())
         {
              std::cerr << result.error << '\n';
              return 1;
         }

        for (const auto& rule : result.rules)
            std::cout << rule.extension << " -> "
                      << rule.destination.generic_u8string() << '\n';

        std::cout << "Loaded rules: " << result.rules.size() << '\n';
        return 0;
        }
    if (command == "--save-index" && argc == 4)
    {
        const auto scan = FileScanner{}.scan(fs::u8path(argv[2]));
        if (!scan.rootValid)
        {
            std::cerr << "Invalid folder path.\n";
            return 1;
        }

        if (!scan.complete())
        {
            std::cerr << "Scan incomplete. Index was not saved.\n";
            return 2;
        }

        for (const auto& entry : scan.entries)
            index.add(entry);

        if (!index.saveToFile(fs::u8path(argv[3])))
        {
            std::cerr << "Could not save the index.\n";
            return 1;
        }

        std::cout << "Saved " << index.size() << " file(s).\n";
        return 0;
    }

    if (command == "--load-index" && argc == 3)
    {
        if (!index.loadFromFile(fs::u8path(argv[2])))
        {
            std::cerr << "Could not load the index.\n";
            return 1;
        }

        std::cout << "Loaded " << index.size() << " file(s).\n";
        for (const auto& entry : index.allFiles())
            std::cout << entry.relativePath.generic_u8string() << '\n';

        return 0;
    }
    if (command == "--check-index" && argc == 4)
    {
        if (!index.loadFromFile(fs::u8path(argv[3])))
        {
            std::cerr << "Could not load the index.\n";
            return 1;
        }

        const auto scan = FileScanner{}.scan(fs::u8path(argv[2]));
        if (!scan.rootValid)
        {
            std::cerr << "Invalid folder path.\n";
            return 1;
        }
        if (!scan.complete())
        {
            std::cerr << "Scan incomplete. Cannot compare the index reliably.\n";
            return 2;
        }

        FileIndex current;
        for (const auto& entry : scan.entries)
            current.add(entry);

        std::size_t changed = 0;
        std::size_t missing = 0;
        for (const auto& saved : index.allFiles())
        {
            const auto* now = current.findByPath(saved.relativePath);
            if (!now)
            {
                std::cout << "[MISSING] " << saved.relativePath.generic_u8string() << '\n';
                ++missing;
            }
            else if (saved.sizeBytes != now->sizeBytes ||
                     saved.modifiedTime != now->modifiedTime)
            {
                std::cout << "[CHANGED] " << saved.relativePath.generic_u8string() << '\n';
                ++changed;
            }
        }

        std::cout << "Changed: " << changed << ", Missing: " << missing << '\n';
        return 0;
    }
        if (command == "--find-name" && argc == 4)
    {
        folderflow::FileIndex searchIndex;
        if (!searchIndex.loadFromFile(fs::u8path(argv[2])))
        {
            std::cerr << "Could not load the index.\n";
            return 1;
        }

        const auto matches = searchIndex.findByName(argv[3]);
        for (const auto* file : matches)
        {
            std::cout << file->relativePath.generic_u8string() << '\n';
        }

        std::cout << "Matches: " << matches.size() << '\n';
        return 0;
    }
        if (command == "--index-summary" && argc == 3)
    {
        folderflow::FileIndex summaryIndex;
        if (!summaryIndex.loadFromFile(fs::u8path(argv[2])))
        {
            std::cerr << "Could not load the index.\n";
            return 1;
        }

        std::cout << "Total files: " << summaryIndex.size() << '\n';
        return 0;
    }
    std::cerr << "Usage:\n"
              << "  folderflow --save-index <folder> <index-file>\n"
              << "  folderflow --load-index <index-file>\n"
              << "  folderflow --check-index <folder> <index-file>\n"
              << "  folderflow --find-name <index-file> <name-part>\n"
              << "  folderflow --index-summary <index-file>\n"
              << "  folderflow --check-rules <config-file>\n";
    return 1;
}