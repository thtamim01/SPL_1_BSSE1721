#include "../src/duplicate_detector.hpp"
#include "../src/scanner.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace
{
namespace fs = std::filesystem;
void check(bool ok, const char* message)
{
    if (!ok)
        throw std::runtime_error(message);
}
struct Fixture
{
    fs::path root;
    Fixture()
    {
        const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int i = 0; i < 30; ++i)
        {
            const auto path = fs::temp_directory_path() /
                ("FolderFlow-duplicates-" + std::to_string(tick) + "-" + std::to_string(i));
            if (fs::create_directory(path))
            {
                root = path;
                return;
            }
        }
        throw std::runtime_error("Could not create test directory");
    }
    ~Fixture()
    {
        std::error_code ec;
        fs::remove_all(root, ec);
    }
};
void write(const fs::path& path, const std::string& data)
{
    std::ofstream stream(path, std::ios::binary);
    stream.write(data.data(), static_cast<std::streamsize>(data.size()));
    stream.close();
    check(static_cast<bool>(stream), "Fixture write failed");
}
template <typename Body>
void test(const char* name, const Body& body, int& pass, int& fail)
{
    try
    {
        body();
        ++pass;
        std::cout << "PASS " << name << '\n';
    }
    catch (const std::exception& error)
    {
        ++fail;
        std::cerr << "FAIL " << name << ": " << error.what() << '\n';
    }
}
}

int main()
{
    int passed = 0, failed = 0;
    test("different names and nested paths share a confirmed group", []
    {
        Fixture fixture;
        fs::create_directory(fixture.root / "nested");
        write(fixture.root / "report.pdf", "same");
        write(fixture.root / "nested" / "different.txt", "same");
        write(fixture.root / "other.txt", "else");
        const auto report = folderflow::detectDuplicates(folderflow::FileScanner{}.scan(fixture.root));
        check(report.issues.empty() && report.groups.size() == 1 &&
              report.groups[0].files.size() == 2 && report.reclaimableBytes == 4,
              "Wrong confirmed group");
    }, passed, failed);
    test("empty files are duplicates with zero reclaimable bytes", []
    {
        Fixture fixture;
        write(fixture.root / "a", "");
        write(fixture.root / "b", "");
        const auto report = folderflow::detectDuplicates(folderflow::FileScanner{}.scan(fixture.root));
        check(report.groups.size() == 1 && report.reclaimableBytes == 0, "Empty files missing");
    }, passed, failed);
    test("equal size different content is not a duplicate", []
    {
        Fixture fixture;
        write(fixture.root / "a", "abcd");
        write(fixture.root / "b", "abce");
        const auto report = folderflow::detectDuplicates(folderflow::FileScanner{}.scan(fixture.root));
        check(report.groups.empty() && report.reclaimableBytes == 0, "False duplicate");
    }, passed, failed);
    test("forced hash collisions still require full byte equality", []
    {
        Fixture fixture;
        write(fixture.root / "a", "abcd");
        write(fixture.root / "b", "abce");
        write(fixture.root / "c", "abcd");
        const auto report = folderflow::detectDuplicates(folderflow::FileScanner{}.scan(fixture.root),
                                                         [](std::uint64_t) { return 1; });
        check(report.groups.size() == 1 && report.groups[0].files.size() == 2 &&
              report.reclaimableBytes == 4, "Hash collision reported as duplicate");
    }, passed, failed);
    test("large files use bounded chunk reads and unchanged contents", []
    {
        Fixture fixture;
        const std::string bytes(200000, 'x');
        write(fixture.root / "a", bytes);
        write(fixture.root / "b", bytes);
        const auto report = folderflow::detectDuplicates(folderflow::FileScanner{}.scan(fixture.root));
        check(report.groups.size() == 1 && report.reclaimableBytes == bytes.size() &&
              fs::file_size(fixture.root / "a") == bytes.size(), "Large file mismatch");
        std::ifstream input(fixture.root / "b", std::ios::binary);
        check(std::string(std::istreambuf_iterator<char>(input), {}) == bytes,
              "Detection altered file contents");
    }, passed, failed);
    test("changed file is reported as unverifiable", []
    {
        Fixture fixture;
        write(fixture.root / "a", "same");
        write(fixture.root / "b", "same");
        const auto scan = folderflow::FileScanner{}.scan(fixture.root);
        write(fixture.root / "b", "modified");
        const auto report = folderflow::detectDuplicates(scan);
        check(report.groups.empty() && !report.issues.empty(), "Changed file silently accepted");
    }, passed, failed);
    test("scanner issues and unknown size remain visible", []
    {
        Fixture fixture;
        write(fixture.root / "a", "same");
        auto scan = folderflow::FileScanner{}.scan(fixture.root);
        scan.issues.push_back({"unreadable", "scan", "permission denied"});
        scan.entries[0].sizeBytes.reset();
        const auto report = folderflow::detectDuplicates(scan);
        check(report.groups.empty() && report.issues.size() == 2, "Incomplete scan hidden");
    }, passed, failed);
    test("file symlinks are skipped", []
    {
        Fixture fixture;
        write(fixture.root / "a", "same");
        write(fixture.root / "b", "same");
        std::error_code ec;
        fs::create_symlink(fixture.root / "a", fixture.root / "link", ec);
        if (ec)
        {
            std::cout << "SKIP symlink fixture: " << ec.message() << '\n';
            return;
        }
        const auto report = folderflow::detectDuplicates(folderflow::FileScanner{}.scan(fixture.root));
        check(report.groups.size() == 1 && report.groups[0].files.size() == 2,
              "Unsafe link was considered");
    }, passed, failed);
    std::cout << "DUPLICATE RESULT: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
