#include "application.hpp"
#include "scanner.hpp"
#include "file_index.hpp"

#include <iomanip>
#include <istream>
#include <ostream>
#include <string>

namespace folderflow
{
int runApplication(std::istream& input, std::ostream& output)
{
    return runApplication(input, output, FileScanner{});
}

int runApplication(std::istream& input, std::ostream& output, const FileScanner& scanner)
{
    output << "FolderFlow Guardian\nSPL-1 Project\n\nEnter folder path: ";
    std::string folderPath;
    if (!std::getline(input, folderPath))
    {
        output << "No folder path input received.\n";
        return 1;
    }
    if (folderPath.empty()) { folderPath = "."; }

    const auto result = scanner.scan(fs::u8path(folderPath));
    if (!result.rootValid) { output << "Invalid folder path!\n"; }
    FileIndex index;
    for (const auto& entry : result.entries)
    {
        index.add(entry);
        switch (entry.kind)
        {
        case EntryKind::Directory: output << "[FOLDER] "; break;
        case EntryKind::RegularFile: output << "[FILE] "; break;
        case EntryKind::SkippedLink: output << "[SKIPPED LINK] "; break;
        case EntryKind::SkippedOther: output << "[SKIPPED OTHER] "; break;
        }
        output << entry.relativePath.generic_u8string();
        if (entry.kind == EntryKind::RegularFile)
        {
            if (entry.sizeBytes) { output << " (" << *entry.sizeBytes << " bytes)"; }
            else { output << " [size unavailable]"; }
        }
        output << '\n';
    }
    for (const auto& issue : result.issues)
    {
        output << "[ERROR] " << issue.relativePath.generic_u8string()
               << " - " << issue.operation << ": " << issue.message << '\n';
    }
    if (!result.rootValid) { return 1; }

    output << "\nTotal Files: " << result.files
           << "\nTotal Folders: " << result.folders
           << "\nTotal Size: " << result.totalBytes << " bytes\n"
           << std::fixed << std::setprecision(2)
           << "Total Size (KB): " << result.totalBytes / 1000.0 << '\n'
           << "Total Size (MB): " << result.totalBytes / 1000000.0 << '\n';
    output << "Indexed Files: " << index.size() << '\n';
    if (!result.complete())
    {
        output << "Warning: scan incomplete (" << result.issues.size()
               << " issue(s)). Totals cover only successfully measured entries.\n";
        return 2;
    }
    return 0;
}
}
