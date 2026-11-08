
#include "file_index.hpp"
#include "rule_config.hpp"
#include "application.hpp"
#include "file_index.hpp"
#include "scanner.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    using namespace folderflow;

    if (argc == 1)
        return runApplication(std::cin, std::cout);

    const std::string command = argv[1];
    FileIndex index;
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