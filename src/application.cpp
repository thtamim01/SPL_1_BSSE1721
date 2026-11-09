#include "application.hpp"
#include "scanner.hpp"
#include "file_index.hpp"
#include "storage_analyzer.hpp"

#include <iomanip>
#include <istream>
#include <ostream>
#include <string>

namespace folderflow
{
int runStorageAnalysis(const fs::path& root, std::size_t limit,
                       std::ostream& output, std::ostream& errors)
{
    const auto scan = FileScanner{}.scan(root);
    for (const auto& issue : scan.issues)
    {
        errors << "[ERROR] " << issue.relativePath.generic_u8string()
               << " - " << issue.operation << ": " << issue.message << '\n';
    }
    if (!scan.rootValid)
        return 1;

    const auto report = analyzeStorage(scan, limit);
    if (!report)
    {
        errors << "Storage analysis stopped: byte total exceeds the supported range.\n";
        return 2;
    }

    const auto printSize = [&](std::uintmax_t bytes)
    {
        output << bytes << " bytes (" << formatStorageSize(bytes) << ')';
    };
    output << "Files: " << report->files << "\nMeasured storage: ";
    printSize(report->bytes);
    output << "\nFiles with unknown size: " << report->unknownSizes << '\n';

    const auto previousFlags = output.flags();
    const auto previousPrecision = output.precision();
    output << "\nBy extension (share of measured storage):\n"
           << std::fixed << std::setprecision(2);
    if (report->byExtension.empty())
        output << "  (none)\n";
    for (const auto& [extension, group] : report->byExtension)
    {
        output << "  " << (extension.empty() ? "(no extension)" : extension)
               << " | " << group.files << " file(s) | ";
        printSize(group.bytes);
        output << " | " << storagePercent(group.bytes, report->bytes) << '%';
        if (group.unknownSizes != 0)
            output << " | " << group.unknownSizes << " unknown size(s)";
        output << '\n';
    }
    output.flags(previousFlags);
    output.precision(previousPrecision);

    const auto printRanking = [&](const char* heading, const std::vector<StorageItem>& items)
    {
        output << '\n' << heading << " (up to " << limit << "):\n";
        if (items.empty())
            output << "  (none)\n";
        for (const auto& item : items)
        {
            output << "  " << item.relativePath << " | ";
            printSize(item.bytes);
            output << '\n';
        }
    };
    printRanking("Largest files", report->largestFiles);
    printRanking("Largest folders by measured subtree size", report->largestFolders);

    if (!scan.complete())
    {
        errors << "Warning: scan incomplete. Rankings and percentages cover measured data only.\n";
        return 2;
    }
    return 0;
}

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

    return runFolderScan(fs::u8path(folderPath), output, scanner);
}

int runFolderScan(const fs::path& root, std::ostream& output, const FileScanner& scanner)
{
    const auto result = scanner.scan(root);
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
