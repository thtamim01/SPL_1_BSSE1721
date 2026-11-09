#include "cli_dispatcher.hpp"

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
#include "application.hpp"
#include "menu.hpp"
#include "scanner.hpp"
#include "duplicate_detector.hpp"
#include <cctype>
#include <charconv>
#include <istream>
#include <ostream>
#include <string>
#include <map>
#include <vector>

namespace folderflow
{
namespace
{
int runCommand(int argc, const char* const argv[], std::istream& consoleInput,
               std::ostream& consoleOutput, std::ostream& consoleErrors)
{
    const std::string command = argv[1];

    if (command == "--find-duplicates")
    {
        if (argc != 3 || *argv[2] == '\0')
        {
            consoleErrors << "Usage: folderflow --find-duplicates <folder>\n";
            return 1;
        }
        return runDuplicateDetection(fs::u8path(argv[2]), consoleOutput, consoleErrors);
    }

    if (command == "--scan")
    {
        if (argc == 2)
            return runApplication(consoleInput, consoleOutput);
        if (argc == 3 && *argv[2] != '\0')
            return runFolderScan(fs::u8path(argv[2]), consoleOutput, FileScanner{});
        consoleErrors << "Usage: folderflow --scan [folder]\n";
        return 1;
    }

    if (command == "--analyze-storage")
    {
        if (argc != 3 && argc != 4)
        {
            consoleErrors << "Usage: folderflow --analyze-storage <folder> [top-count]\n";
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
                consoleErrors << "Top count must be an integer from 1 to 100.\n";
                return 1;
            }
        }
        return runStorageAnalysis(fs::u8path(argv[2]), limit, consoleOutput, consoleErrors);
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
            consoleErrors << "Usage: folderflow --undo <folder>\n";
            return 1;
        }

#ifdef _WIN32
        std::error_code ec;
        const auto root = fs::absolute(fs::u8path(argv[2]), ec).lexically_normal();
        const auto journalDirectory = root / ".folderflow-undo";
        const auto journalPath = journalDirectory / "moves.log";
        if (ec || !plainDirectoryChain(journalDirectory))
        {
            consoleErrors << "No accessible move journal for this folder.\n";
            return 1;
        }

        const DWORD journalAttributes = GetFileAttributesW(journalPath.c_str());
        if (journalAttributes == INVALID_FILE_ATTRIBUTES ||
            (journalAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
        {
            consoleErrors << "Cannot open the move journal safely.\n";
            return 1;
        }

        std::ifstream input(journalPath, std::ios::binary);
        std::string line;
        if (!std::getline(input, line) || line != "FOLDERFLOW_MOVE_JOURNAL_V1")
        {
            consoleErrors << "Unrecognized move journal.\n";
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
                consoleErrors << "Unexpected record after cleanup marker.\n";
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
                    consoleErrors << "Invalid folder record.\n";
                    return 2;
                }

                const auto folder = fs::u8path(name);
                if (!safeRelativePath(folder) || folder.has_parent_path())
                {
                    consoleErrors << "Invalid cleanup path.\n";
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
                consoleErrors << "Incomplete or invalid journal. Undo stopped.\n";
                return 2;
            }

            if (result == "MOVED")
            {
                const auto original = fs::u8path(originalText);
                const auto destination = fs::u8path(destinationText);
                if (!safeRelativePath(original) || !safeRelativePath(destination) ||
                    original.filename() != destination.filename())
                {
                    consoleErrors << "Invalid paths in the move journal.\n";
                    return 2;
                }
                moves.push_back({original, destination});
            }
        }

        if (input.bad() || !input.eof())
        {
            consoleErrors << "Cannot read the complete move journal.\n";
            return 2;
        }
        input.close();

        if (moves.empty() && (createdFolders.empty() || cleanupRecorded))
        {
            consoleOutput << "Nothing to undo.\n";
            return 0;
        }

        std::ofstream progress(journalPath, std::ios::binary | std::ios::app);
        if (!progress)
        {
            consoleErrors << "Cannot update the move journal. Undo stopped.\n";
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
                consoleErrors << "Undo blocked by a missing or unsafe folder.\n";
                return 2;
            }

            const DWORD attributes = GetFileAttributesW(destination.c_str());
            if (attributes == INVALID_FILE_ATTRIBUTES ||
                (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
            {
                consoleErrors << "Undo blocked: moved file is missing or unsafe.\n";
                return 2;
            }

            const bool isJournal = fs::equivalent(destination, journalPath, ec);
            if (ec || isJournal)
            {
                consoleErrors << "Undo blocked: cannot verify the moved file.\n";
                return 2;
            }

            if (!MoveFileExW(destination.c_str(), original.c_str(), 0))
            {
                const DWORD error = GetLastError();
                consoleErrors << "Undo blocked at " << move.original.generic_u8string()
                          << " (Windows error " << error << "). Record kept.\n";
                return 2;
            }

            progress << "UNDONE\n";
            progress.flush();
            if (!progress)
            {
                consoleErrors << "File restored, but the journal update failed. "
                             "Inspect the paths before retrying.\n";
                return 2;
            }

            consoleOutput << "RESTORED: " << move.destination.generic_u8string()
                      << " -> " << move.original.generic_u8string() << '\n';
            moves.pop_back();
        }

        if (!createdFolders.empty() && !cleanupRecorded)
        {
            progress << "CLEANUP\n" << std::flush;
            if (!progress)
            {
                consoleErrors << "Cannot record cleanup. Folders kept.\n";
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
                    consoleOutput << "REMOVED EMPTY FOLDER: " << folder.generic_u8string() << '\n';
                else
                    consoleOutput << "KEPT FOLDER: " << folder.generic_u8string() << '\n';
            }
        }

        return 0;
#else
        consoleErrors << "--undo is currently supported on Windows only.\n";
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
            consoleErrors << config.error << '\n';
            return 1;
        }

        const auto scan = FileScanner{}.scan(fs::u8path(argv[2]));
        if (!scan.rootValid)
        {
            consoleErrors << "Invalid folder path.\n";
            return 1;
        }
        if (!scan.complete())
        {
            consoleErrors << "Scan incomplete. Rule evaluation stopped.\n";
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

            consoleOutput << entry.relativePath.generic_u8string();

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
                        consoleOutput << " -> " << destinationPath
                                  << " [SKIP: destination folder is a junction or other reparse point]\n";
                        continue;
                    }
                }
#endif
                consoleOutput << " -> " << destinationPath;

                if (command == "--dry-run")
                {
                    std::error_code ec;
                    const bool occupied = fs::exists(scan.root / destination, ec);

                    if (ec)
                        consoleOutput << " [SKIP: cannot check destination]";
                    else if (occupied)
                        consoleOutput << " [SKIP: destination already exists]";
                    else
                        consoleOutput << " [DRY RUN: move proposed]";
                }

                consoleOutput << '\n';
            }
            else
            {
                consoleOutput << " -> no matching rule\n";
            }
        }

        for (const auto& [key, group] : destinations)
        {
            if (group.sources.size() < 2)
                continue;

            consoleOutput << "Conflict: " << group.displayPath << '\n';
            for (const auto& source : group.sources)
                consoleOutput << "  - " << source << '\n';
        }

        if (command == "--organize")
        {
#ifdef _WIN32
            std::error_code ec;
            const auto root = fs::absolute(scan.root, ec).lexically_normal();
            if (ec)
            {
                consoleErrors << "Cannot resolve the selected folder.\n";
                return 1;
            }

            if (!plainDirectoryChain(root))
            {
                consoleErrors << "Selected folder has an inaccessible or linked parent.\n";
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
                    consoleErrors << "Cannot create the move journal. "
                                 "An earlier journal may already exist.\n";
                    return false;
                }

                journal.open(journalDirectory / "moves.log", std::ios::binary);
                journal << "FOLDERFLOW_MOVE_JOURNAL_V1\n" << std::flush;
                if (!journal)
                    consoleErrors << "Cannot write the move journal.\n";
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
                    consoleOutput << "SKIP: invalid destination folder for "
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
                    consoleOutput << "SKIP: unsafe source " << group.sources.front() << '\n';
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
                                consoleErrors << "Cannot record the new folder. Organization stopped.\n";
                                return 2;
                            }
                        }
                        if (ec)
                        {
                            consoleErrors << "FAILED: cannot create " << folder.generic_u8string()
                                      << ": " << ec.message() << '\n';
                            failed = true;
                            continue;
                        }
                    }
                }

                if (!plainDirectoryChain(targetFolder))
                {
                    consoleOutput << "SKIP: unsafe destination folder for " << group.displayPath << '\n';
                    continue;
                }

                if (GetFileAttributesW(target.c_str()) != INVALID_FILE_ATTRIBUTES)
                {
                    consoleOutput << "SKIP: destination already exists " << group.displayPath << '\n';
                    continue;
                }

                if (!openJournal())
                    return 2;

                journal << "PENDING " << std::quoted(group.sources.front())
                        << ' ' << std::quoted(group.displayPath) << '\n';
                journal.flush();
                if (!journal)
                {
                    consoleErrors << "Cannot save the move record. Move stopped.\n";
                    return 2;
                }

                const bool moved = MoveFileExW(source.c_str(), target.c_str(), 0) != 0;
                const DWORD error = moved ? ERROR_SUCCESS : GetLastError();

                journal << (moved ? "MOVED\n" : "FAILED\n");
                journal.flush();
                if (!journal)
                {
                    consoleErrors << "Cannot save the move result for "
                              << group.sources.front() << " -> " << group.displayPath
                              << ". Inspect the paths before retrying.\n";
                    return 2;
                }

                if (!moved)
                {
                    consoleErrors << "FAILED: " << group.sources.front()
                              << " (Windows error " << error << ")\n";
                    failed = true;
                    continue;
                }

                consoleOutput << "MOVED: " << group.sources.front()
                          << " -> " << group.displayPath << '\n';
            }

            return failed ? 2 : 0;
#else
            consoleErrors << "--organize is currently supported on Windows only.\n";
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
              consoleErrors << result.error << '\n';
              return 1;
         }

        for (const auto& rule : result.rules)
            consoleOutput << rule.extension << " -> "
                      << rule.destination.generic_u8string() << '\n';

        consoleOutput << "Loaded rules: " << result.rules.size() << '\n';
        return 0;
        }
    if (command == "--save-index" && argc == 4)
    {
        const auto scan = FileScanner{}.scan(fs::u8path(argv[2]));
        if (!scan.rootValid)
        {
            consoleErrors << "Invalid folder path.\n";
            return 1;
        }

        if (!scan.complete())
        {
            consoleErrors << "Scan incomplete. Index was not saved.\n";
            return 2;
        }

        for (const auto& entry : scan.entries)
            index.add(entry);

        if (!index.saveToFile(fs::u8path(argv[3])))
        {
            consoleErrors << "Could not save the index.\n";
            return 1;
        }

        consoleOutput << "Saved " << index.size() << " file(s).\n";
        return 0;
    }

    if (command == "--load-index" && argc == 3)
    {
        if (!index.loadFromFile(fs::u8path(argv[2])))
        {
            consoleErrors << "Could not load the index.\n";
            return 1;
        }

        consoleOutput << "Loaded " << index.size() << " file(s).\n";
        for (const auto& entry : index.allFiles())
            consoleOutput << entry.relativePath.generic_u8string() << '\n';

        return 0;
    }
    if (command == "--check-index" && argc == 4)
    {
        if (!index.loadFromFile(fs::u8path(argv[3])))
        {
            consoleErrors << "Could not load the index.\n";
            return 1;
        }

        const auto scan = FileScanner{}.scan(fs::u8path(argv[2]));
        if (!scan.rootValid)
        {
            consoleErrors << "Invalid folder path.\n";
            return 1;
        }
        if (!scan.complete())
        {
            consoleErrors << "Scan incomplete. Cannot compare the index reliably.\n";
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
                consoleOutput << "[MISSING] " << saved.relativePath.generic_u8string() << '\n';
                ++missing;
            }
            else if (saved.sizeBytes != now->sizeBytes ||
                     saved.modifiedTime != now->modifiedTime)
            {
                consoleOutput << "[CHANGED] " << saved.relativePath.generic_u8string() << '\n';
                ++changed;
            }
        }

        consoleOutput << "Changed: " << changed << ", Missing: " << missing << '\n';
        return 0;
    }
        if (command == "--find-name" && argc == 4)
    {
        folderflow::FileIndex searchIndex;
        if (!searchIndex.loadFromFile(fs::u8path(argv[2])))
        {
            consoleErrors << "Could not load the index.\n";
            return 1;
        }

        const auto matches = searchIndex.findByName(argv[3]);
        for (const auto* file : matches)
        {
            consoleOutput << file->relativePath.generic_u8string() << '\n';
        }

        consoleOutput << "Matches: " << matches.size() << '\n';
        return 0;
    }
        if (command == "--index-summary" && argc == 3)
    {
        folderflow::FileIndex summaryIndex;
        if (!summaryIndex.loadFromFile(fs::u8path(argv[2])))
        {
            consoleErrors << "Could not load the index.\n";
            return 1;
        }

        consoleOutput << "Total files: " << summaryIndex.size() << '\n';
        return 0;
    }
    consoleErrors << "Usage:\n"
              << "  folderflow                  (interactive menu)\n"
              << "  folderflow --scan [folder]\n"
              << "  folderflow --analyze-storage <folder> [top-count]\n"
              << "  folderflow --find-duplicates <folder>\n"
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

}

int runCli(int argc, const char* const argv[], std::istream& consoleInput,
           std::ostream& consoleOutput, std::ostream& consoleErrors)
{
    if (argc > 1)
        return runCommand(argc, argv, consoleInput, consoleOutput, consoleErrors);

#ifdef _WIN32
    constexpr bool fileMovesAvailable = true;
#else
    constexpr bool fileMovesAvailable = false;
#endif

    return runMenu(consoleInput, consoleOutput,
        [&](const std::vector<std::string>& arguments)
        {
            std::vector<const char*> commandArgs{argv[0]};
            for (const auto& argument : arguments)
                commandArgs.push_back(argument.c_str());
            return runCommand(static_cast<int>(commandArgs.size()), commandArgs.data(),
                              consoleInput, consoleOutput, consoleErrors);
        }, fileMovesAvailable);
}
}
