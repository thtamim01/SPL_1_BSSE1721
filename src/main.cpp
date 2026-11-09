#include <sstream>
#include <fstream>
#include <iomanip>
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
#include "menu.hpp"
#include "file_index.hpp"
#include "scanner.hpp"
#include <cctype>
#include <charconv>
#include <iostream>
#include <string>
#include <map>
#include <vector>

static int runCommand(int argc, const char* const argv[])
{
    using namespace folderflow;

    const std::string command = argv[1];

    if (command == "--scan")
    {
        if (argc == 2)
            return runApplication(std::cin, std::cout);
        if (argc == 3 && *argv[2] != '\0')
            return runFolderScan(fs::u8path(argv[2]), std::cout, FileScanner{});
        std::cerr << "Usage: folderflow --scan [folder]\n";
        return 1;
    }

    if (command == "--analyze-storage")
    {
        if (argc != 3 && argc != 4)
        {
            std::cerr << "Usage: folderflow --analyze-storage <folder> [top-count]\n";
            return 1;
        }
        std::size_t limit = 5;
        if (argc == 4)
        {
            const std::string count = argv[3];
            const auto end = count.data() + count.size();
            const auto parsed = std::from_chars(count.data(), end, limit);
            if (parsed.ec != std::errc{} || parsed.ptr != end || limit < 1 || limit > 100)
            {
                std::cerr << "Top count must be an integer from 1 to 100.\n";
                return 1;
            }
        }
        return runStorageAnalysis(fs::u8path(argv[2]), limit, std::cout, std::cerr);
    }
    #ifdef _WIN32
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

    #endif

    if (command == "--undo")
    {
        if (argc != 3)
        {
            std::cerr << "Usage: folderflow --undo <folder>\n";
            return 1;
        }

#ifdef _WIN32
        std::error_code ec;
        const auto root = fs::absolute(fs::u8path(argv[2]), ec).lexically_normal();
        const auto journalDirectory = root / ".folderflow-undo";
        const auto journalPath = journalDirectory / "moves.log";
        if (ec || !plainDirectoryChain(journalDirectory))
        {
            std::cerr << "No accessible move journal for this folder.\n";
            return 1;
        }

        const DWORD journalAttributes = GetFileAttributesW(journalPath.c_str());
        if (journalAttributes == INVALID_FILE_ATTRIBUTES ||
            (journalAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
        {
            std::cerr << "Cannot open the move journal safely.\n";
            return 1;
        }

        std::ifstream input(journalPath, std::ios::binary);
        std::string line;
        if (!std::getline(input, line) || line != "FOLDERFLOW_MOVE_JOURNAL_V1")
        {
            std::cerr << "Unrecognized move journal.\n";
            return 1;
        }

        struct MoveRecord
        {
            fs::path original;
            fs::path destination;
        };

        const auto safeRelativePath = [](const fs::path& path)
        {
            if (path.empty() || path.has_root_path())
                return false;

            for (const auto& component : path)
            {
                const auto name = component.native();
                if (name.empty() || component == "." || component == ".." ||
                    name.find_first_of(L"<>:\"/\\|?*") != std::wstring::npos ||
                    name.back() == L'.' || name.back() == L' ')
                    return false;
            }
            return true;
        };

        std::vector<MoveRecord> moves;
        std::vector<fs::path> createdFolders;
        bool cleanupRecorded = false;
        bool undoStarted = false;
        while (std::getline(input, line))
        {
            if (cleanupRecorded)
            {
                std::cerr << "Unexpected record after cleanup marker.\n";
                return 2;
            }

            if (line == "CLEANUP" && moves.empty() && !createdFolders.empty())
            {
                cleanupRecorded = true;
                continue;
            }

            if (!undoStarted && line.compare(0, 8, "CREATED ") == 0)
            {
                std::istringstream folderRecord(line.substr(8));
                std::string name;
                if (!(folderRecord >> std::quoted(name)) || !(folderRecord >> std::ws).eof())
                {
                    std::cerr << "Invalid folder record.\n";
                    return 2;
                }

                const auto folder = fs::u8path(name);
                if (!safeRelativePath(folder) || folder.has_parent_path())
                {
                    std::cerr << "Invalid cleanup path.\n";
                    return 2;
                }

                createdFolders.push_back(folder);
                continue;
            }

            if (line == "UNDONE" && !moves.empty())
            {
                undoStarted = true;
                moves.pop_back();
                continue;
            }

            std::istringstream record(line);
            std::string tag, originalText, destinationText, result;
            if (undoStarted || !(record >> tag) || tag != "PENDING" ||
                !(record >> std::quoted(originalText) >> std::quoted(destinationText)) ||
                !(record >> std::ws).eof() || !std::getline(input, result) ||
                (result != "MOVED" && result != "FAILED"))
            {
                std::cerr << "Incomplete or invalid journal. Undo stopped.\n";
                return 2;
            }

            if (result == "MOVED")
            {
                const auto original = fs::u8path(originalText);
                const auto destination = fs::u8path(destinationText);
                if (!safeRelativePath(original) || !safeRelativePath(destination) ||
                    original.filename() != destination.filename())
                {
                    std::cerr << "Invalid paths in the move journal.\n";
                    return 2;
                }
                moves.push_back({original, destination});
            }
        }

        if (input.bad() || !input.eof())
        {
            std::cerr << "Cannot read the complete move journal.\n";
            return 2;
        }
        input.close();

        if (moves.empty() && (createdFolders.empty() || cleanupRecorded))
        {
            std::cout << "Nothing to undo.\n";
            return 0;
        }

        std::ofstream progress(journalPath, std::ios::binary | std::ios::app);
        if (!progress)
        {
            std::cerr << "Cannot update the move journal. Undo stopped.\n";
            return 2;
        }

        while (!moves.empty())
        {
            const auto& move = moves.back();
            const auto original = root / move.original;
            const auto destination = root / move.destination;
            if (!plainDirectoryChain(original.parent_path()) ||
                !plainDirectoryChain(destination.parent_path()))
            {
                std::cerr << "Undo blocked by a missing or unsafe folder.\n";
                return 2;
            }

            const DWORD attributes = GetFileAttributesW(destination.c_str());
            if (attributes == INVALID_FILE_ATTRIBUTES ||
                (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
            {
                std::cerr << "Undo blocked: moved file is missing or unsafe.\n";
                return 2;
            }

            const bool isJournal = fs::equivalent(destination, journalPath, ec);
            if (ec || isJournal)
            {
                std::cerr << "Undo blocked: cannot verify the moved file.\n";
                return 2;
            }

            if (!MoveFileExW(destination.c_str(), original.c_str(), 0))
            {
                const DWORD error = GetLastError();
                std::cerr << "Undo blocked at " << move.original.generic_u8string()
                          << " (Windows error " << error << "). Record kept.\n";
                return 2;
            }

            progress << "UNDONE\n";
            progress.flush();
            if (!progress)
            {
                std::cerr << "File restored, but the journal update failed. "
                             "Inspect the paths before retrying.\n";
                return 2;
            }

            std::cout << "RESTORED: " << move.destination.generic_u8string()
                      << " -> " << move.original.generic_u8string() << '\n';
            moves.pop_back();
        }

        if (!createdFolders.empty() && !cleanupRecorded)
        {
            progress << "CLEANUP\n" << std::flush;
            if (!progress)
            {
                std::cerr << "Cannot record cleanup. Folders kept.\n";
                return 2;
            }

            for (const auto& folder : createdFolders)
            {
                const auto candidate = root / folder;
                if (!plainDirectoryChain(candidate))
                    continue;

                const bool isJournalDirectory = fs::equivalent(candidate, journalDirectory, ec);
                if (ec || isJournalDirectory)
                    continue;

                if (RemoveDirectoryW(candidate.c_str()))
                    std::cout << "REMOVED EMPTY FOLDER: " << folder.generic_u8string() << '\n';
                else
                    std::cout << "KEPT FOLDER: " << folder.generic_u8string() << '\n';
            }
        }

        return 0;
#else
        std::cerr << "--undo is currently supported on Windows only.\n";
        return 1;
#endif
    }

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

            if (!plainDirectoryChain(root))
            {
                std::cerr << "Selected folder has an inaccessible or linked parent.\n";
                return 1;
            }

            const auto journalDirectory = root / ".folderflow-undo";
            std::ofstream journal;
            const auto openJournal = [&]()
            {
                if (journal.is_open())
                    return static_cast<bool>(journal);

                if (!fs::create_directory(journalDirectory, ec))
                {
                    std::cerr << "Cannot create the move journal. "
                                 "An earlier journal may already exist.\n";
                    return false;
                }

                journal.open(journalDirectory / "moves.log", std::ios::binary);
                journal << "FOLDERFLOW_MOVE_JOURNAL_V1\n" << std::flush;
                if (!journal)
                    std::cerr << "Cannot write the move journal.\n";
                return static_cast<bool>(journal);
            };
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
                        if (!openJournal())
                            return 2;
                        const bool created = fs::create_directory(targetFolder, ec);
                        if (created)
                        {
                            journal << "CREATED " << std::quoted(folder.generic_u8string())
                                    << '\n' << std::flush;
                            if (!journal)
                            {
                                std::cerr << "Cannot record the new folder. Organization stopped.\n";
                                return 2;
                            }
                        }
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

                if (!openJournal())
                    return 2;

                journal << "PENDING " << std::quoted(group.sources.front())
                        << ' ' << std::quoted(group.displayPath) << '\n';
                journal.flush();
                if (!journal)
                {
                    std::cerr << "Cannot save the move record. Move stopped.\n";
                    return 2;
                }

                const bool moved = MoveFileExW(source.c_str(), target.c_str(), 0) != 0;
                const DWORD error = moved ? ERROR_SUCCESS : GetLastError();

                journal << (moved ? "MOVED\n" : "FAILED\n");
                journal.flush();
                if (!journal)
                {
                    std::cerr << "Cannot save the move result for "
                              << group.sources.front() << " -> " << group.displayPath
                              << ". Inspect the paths before retrying.\n";
                    return 2;
                }

                if (!moved)
                {
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
              << "  folderflow                  (interactive menu)\n"
              << "  folderflow --scan [folder]\n"
              << "  folderflow --analyze-storage <folder> [top-count]\n"
              << "  folderflow --save-index <folder> <index-file>\n"
              << "  folderflow --load-index <index-file>\n"
              << "  folderflow --check-index <folder> <index-file>\n"
              << "  folderflow --find-name <index-file> <name-part>\n"
              << "  folderflow --index-summary <index-file>\n"
              << "  folderflow --check-rules <config-file>\n"
              << "  folderflow --evaluate-rules <folder> <config-file>\n"
              << "  folderflow --dry-run <folder> <config-file>\n"
              << "  folderflow --organize <folder> <config-file>\n"
              << "  folderflow --undo <folder>\n";
    return 1;
}

int main(int argc, char* argv[])
{
    if (argc > 1)
        return runCommand(argc, argv);

#ifdef _WIN32
    constexpr bool fileMovesAvailable = true;
#else
    constexpr bool fileMovesAvailable = false;
#endif

    return folderflow::runMenu(std::cin, std::cout,
        [&](const std::vector<std::string>& arguments)
        {
            std::vector<const char*> commandArgs{argv[0]};
            for (const auto& argument : arguments)
                commandArgs.push_back(argument.c_str());
            return runCommand(static_cast<int>(commandArgs.size()), commandArgs.data());
        }, fileMovesAvailable);
}
