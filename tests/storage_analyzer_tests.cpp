#include "../src/storage_analyzer.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

using namespace folderflow;

namespace
{
void check(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

FileMetadata file(const fs::path& path, std::optional<std::uintmax_t> bytes)
{
    return {path, EntryKind::RegularFile, path.extension().u8string(), bytes, {}};
}

FileMetadata directory(const fs::path& path)
{
    return {path, EntryKind::Directory, {}, {}, {}};
}

StorageReport analyze(std::vector<FileMetadata> entries, std::size_t limit = 5)
{
    ScanResult scan;
    scan.entries = std::move(entries);
    return analyzeStorage(scan, limit).value();
}
}

int main()
{
    int passed = 0;
    int failed = 0;
    const auto test = [&](const char* name, auto run)
    {
        try
        {
            run();
            ++passed;
            std::cout << "PASS " << name << '\n';
        }
        catch (const std::exception& error)
        {
            ++failed;
            std::cerr << "FAIL " << name << ": " << error.what() << '\n';
        }
    };

    test("empty scan", []
    {
        const auto report = analyze({});
        check(report.files == 0 && report.bytes == 0, "empty totals");
        check(report.byExtension.empty() && report.largestFiles.empty() &&
              report.largestFolders.empty(), "empty report");
    });

    test("extension totals, counts and percentages", []
    {
        const auto report = analyze({file("a.PDF", 125), file("b.pdf", 125),
                                     file("c.jpg", 750), file("README", 0)});
        const auto& pdf = report.byExtension.at(".pdf");
        check(report.byExtension.size() == 3, "expected exactly three extension groups");
        check(report.files == 4 && report.bytes == 1000, "file count and total");
        check(pdf.files == 2 && pdf.bytes == 250, "PDF case folding and totals");
        check(storagePercent(pdf.bytes, report.bytes) == 25.0L, "PDF share");
        check(report.byExtension.at("").files == 1, "extensionless group");
    });

    test("unknown size differs from zero", []
    {
        const auto report = analyze({file("a.txt", std::nullopt), file("empty.txt", 0)});
        const auto& text = report.byExtension.at(".txt");
        check(report.files == 2 && report.unknownSizes == 1, "unknown size still counts as a file");
        check(text.files == 2 && text.unknownSizes == 1, "group unknown count");
        check(report.largestFiles.size() == 1, "only measured files may be ranked");
        check(report.largestFiles.front().relativePath == "empty.txt", "zero is measurable");
        check(storagePercent(text.bytes, report.bytes) == 0.0L, "zero total avoids division");
    });

    test("subtree totals, empty folders and excluded entry kinds", []
    {
        const auto report = analyze({directory("A"), directory("A/Deep"),
                                     directory("B"), directory("C"),
                                     file("root.bin", 900), file("A/a.bin", 40),
                                     file("A/Deep/b.bin", 60), file("B/c.bin", 70),
                                     {"fake.txt", EntryKind::SkippedLink, ".txt", 1000, {}},
                                     {"pipe", EntryKind::SkippedOther, {}, 1000, {}}});
        const auto& folders = report.largestFolders;
        check(report.files == 4 && report.bytes == 1070, "only regular files count");
        check(folders.size() == 4, "root excluded; empty folder included");
        check(folders[0].relativePath == "A" && folders[0].bytes == 100, "parent subtree");
        check(folders[1].relativePath == "B" && folders[1].bytes == 70, "separate subtree");
        check(folders[2].relativePath == "A/Deep" && folders[2].bytes == 60, "nested subtree");
        check(folders[3].relativePath == "C" && folders[3].bytes == 0, "empty folder");
    });

    test("bounded heap, late replacements and ties", []
    {
        const auto report = analyze({file("z/large.bin", 300), file("m/file.bin", 140),
                                     file("q/file.bin", 50), file("p/file.bin", 10),
                                     file("a/large.bin", 300), file("b/file.bin", 200),
                                     file("c/file.bin", 199)}, 3);
        const auto& files = report.largestFiles;
        const auto& folders = report.largestFolders;
        check(files.size() == 3 && folders.size() == 3, "enforce requested cap");
        check(files[0].relativePath == "a/large.bin" && files[1].relativePath == "z/large.bin" &&
              files[2].relativePath == "b/file.bin", "rank files by bytes then path");
        check(folders[0].relativePath == "a" && folders[1].relativePath == "z" &&
              folders[2].relativePath == "b", "rank folders by bytes then path");
    });

    test("ties at the top-count cutoff", []
    {
        const auto report = analyze({file("z/item", 42), file("b/item", 42),
                                     file("a/item", 42), file("x/item", 42)}, 1);
        check(report.largestFiles.size() == 1 && report.largestFiles[0].relativePath == "a/item",
              "retain the first path alphabetically");
        check(report.largestFolders.size() == 1 && report.largestFolders[0].relativePath == "a",
              "apply the same tie rule to folders");
    });

    test("zero ranking limit for module callers", []
    {
        const auto report = analyze({file("a/b.bin", 10)}, 0);
        check(report.files == 1 && report.bytes == 10, "totals are still computed");
        check(report.largestFiles.empty() && report.largestFolders.empty(), "empty rankings");
    });

    test("large sizes remain exact", []
    {
        constexpr std::uintmax_t large = 5'000'000'000;
        const auto report = analyze({file("x/big.bin", large), file("x/small.bin", 1)});
        check(report.bytes == large + 1, "wide integer total");
        check(report.byExtension.at(".bin").bytes == large + 1, "wide extension total");
        check(report.largestFolders.front().bytes == large + 1, "wide folder total");
        check(formatStorageSize(large) == "5.00 GB", "large display");
    });

    test("maximum total is allowed and overflow is refused", []
    {
        const auto max = std::numeric_limits<std::uintmax_t>::max();
        ScanResult scan;
        scan.entries = {file("a/b.bin", max - 1), file("a/c.bin", 1)};
        check(analyzeStorage(scan, 2).value().bytes == max, "exact maximum");
        scan.entries.push_back(file("other.txt", 1));
        check(!analyzeStorage(scan, 2), "reject total overflow");
    });

    test("decimal unit boundaries", []
    {
        check(formatStorageSize(0) == "0 B", "zero bytes");
        check(formatStorageSize(999) == "999 B", "just below KB");
        check(formatStorageSize(1000) == "1.00 KB", "KB boundary");
        check(formatStorageSize(1'500'000) == "1.50 MB", "fractional MB");
        check(formatStorageSize(1'000'000'000) == "1.00 GB", "GB boundary");
    });

    std::cout << passed << " passed, " << failed << " failed, 0 skipped\n";
    return failed == 0 ? 0 : 1;
}
