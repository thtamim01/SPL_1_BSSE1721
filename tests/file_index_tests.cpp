#include "../src/file_index.hpp"
#include <functional>
#include <iostream>
#include <stdexcept>

using namespace folderflow;
void check(bool condition) { if (!condition) throw std::runtime_error("check failed"); }
FileMetadata file(const fs::path& path, std::string extension, std::uintmax_t bytes = 10)
{
    FileMetadata result;
    result.relativePath = path;
    result.kind = EntryKind::RegularFile;
    result.extension = extension;
    result.sizeBytes = bytes;
    return result;
}
int main()
{
    int passed = 0, failed = 0;
    auto test = [&](const char* name, const std::function<void()>& body) {
        try { body(); ++passed; std::cout << "PASS: " << name << '\n'; }
        catch (const std::exception& error) { ++failed; std::cout << "FAIL: " << name << ": " << error.what() << '\n'; }
    };
    test("empty index and missing queries", [] {
        FileIndex i; check(i.size() == 0 && !i.findByPath("missing") && i.findByExtension("txt").empty());
    });
    test("path lookup retains metadata", [] {
        FileIndex i; check(i.add(file("nested/a.txt", ".txt", 42)));
        auto record = i.findByPath("nested/a.txt"); check(record && record->sizeBytes == 42);
    });
    test("same filename in distinct folders", [] {
        FileIndex i; i.add(file("a/report.txt", ".txt")); i.add(file("b/report.txt", ".txt"));
        check(i.size() == 2 && i.findByExtension("txt").size() == 2);
    });
    test("extension lookup normalizes ASCII case and dot", [] {
        FileIndex i; i.add(file("a.TXT", ".TXT")); i.add(file("b.txt", ".txt"));
        check(i.findByExtension("TXT").size() == 2 && i.findByExtension(".txt").size() == 2);
    });
    test("repeated add replaces without duplicate bucket", [] {
        FileIndex i; i.add(file("a.txt", ".txt")); i.add(file("a.txt", ".txt", 99));
        check(i.size() == 1 && i.findByExtension("txt").size() == 1 && i.findByPath("a.txt")->sizeBytes == 99);
    });
    test("replacement updates extension membership", [] {
        FileIndex i; i.add(file("a", ".txt")); i.add(file("a", ".log"));
        check(i.findByExtension("txt").empty() && i.findByExtension("log").size() == 1);
    });
    test("extensionless file and unavailable size", [] {
        FileIndex i; auto record = file("README", ""); record.sizeBytes.reset(); i.add(record);
        check(i.findByExtension("").size() == 1 && !i.findByPath("README")->sizeBytes);
    });
    test("directories and links rejected", [] {
        FileIndex i; auto record = file("folder", ""); record.kind = EntryKind::Directory;
        check(!i.add(record)); record.kind = EntryKind::SkippedLink; check(!i.add(record) && i.size() == 0);
    });
    test("invalid and escaping keys rejected", [] {
        FileIndex i; check(!i.add(file("", "")) && !i.add(file(".", "")));
        check(!i.add(file("../outside.txt", ".txt")) && !i.add(file(fs::current_path() / "absolute.txt", ".txt")));
        check(i.size() == 0);
    });
    test("normalized relative paths share one record", [] {
        FileIndex i; i.add(file("a/../nested/a.txt", ".txt"));
        check(i.findByPath("nested/./a.txt") && i.size() == 1);
    });
    test("allFiles returns every record in path order as copies", [] {
        FileIndex index;
        index.add(file("b.txt", ".txt", 20));
        index.add(file("a.txt", ".txt", 10));

        auto records = index.allFiles();

        check(records.size() == 2);
        check(records[0].relativePath == fs::path("a.txt"));
        check(records[1].relativePath == fs::path("b.txt"));
        check(records[0].sizeBytes == 10);
        check(records[1].sizeBytes == 20);

        records[0].sizeBytes = 999;
        check(index.findByPath("a.txt")->sizeBytes == 10);
    });
    test("returned pointer survives additional insertions", [] {
        FileIndex i; i.add(file("first.txt", ".txt", 1)); auto first = i.findByPath("first.txt");
        for (int n = 0; n < 100; ++n) i.add(file("item" + std::to_string(n), "", n));
        check(first && first->sizeBytes == 1 && i.size() == 101);
    });
    std::cout << "INDEX RESULT: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
