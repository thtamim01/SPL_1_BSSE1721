#include "../src/application.hpp"
#include "../src/scanner.hpp"

#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef _WIN32
#include <sys/stat.h>
#endif

using namespace folderflow;

namespace
{
struct SkipTest : std::runtime_error { using std::runtime_error::runtime_error; };

void check(bool condition, const std::string& message)
{
    if (!condition) { throw std::runtime_error(message); }
}

class TempDirectory
{
public:
    fs::path path;

    TempDirectory()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int attempt = 0; attempt < 100; ++attempt)
        {
            path = fs::temp_directory_path() /
                   ("folderflow-owned-test-" + std::to_string(stamp) + "-" + std::to_string(attempt));
            std::error_code error;
            if (fs::create_directory(path, error)) { return; }
            if (error) { throw fs::filesystem_error("create test directory", path, error); }
        }
        throw std::runtime_error("Could not allocate a unique fixture directory.");
    }

    ~TempDirectory()
    {
        // path is only a directory exclusively created by this fixture instance.
        std::error_code error;
        fs::remove_all(path, error);
        if (error) { std::cerr << "Fixture cleanup warning: " << error.message() << '\n'; }
    }

    TempDirectory(const TempDirectory&) = delete;
    TempDirectory& operator=(const TempDirectory&) = delete;
};

void writeBytes(const fs::path& path, std::size_t count)
{
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    const std::string bytes(count, 'x');
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!file) { throw std::runtime_error("Could not write fixture: " + path.u8string()); }
}

FileMetadata entry(const ScanResult& result, const std::string& relative)
{
    for (const auto& value : result.entries)
    {
        if (value.relativePath.generic_u8string() == relative) { return value; }
    }
    throw std::runtime_error("Missing record: " + relative);
}

bool hasIssue(const ScanResult& result, const std::string& operation)
{
    for (const auto& issue : result.issues)
    {
        if (issue.operation == operation) { return true; }
    }
    return false;
}

void makeLink(const fs::path& target, const fs::path& link, bool directory = false)
{
    std::error_code error;
    if (directory) { fs::create_directory_symlink(target, link, error); }
    else { fs::create_symlink(target, link, error); }
    if (error) { throw SkipTest("Link fixture unavailable: " + error.message()); }
}

struct PermissionRestore
{
    fs::path path;
    fs::perms permissions;
    ~PermissionRestore()
    {
        std::error_code error;
        fs::permissions(path, permissions, fs::perm_options::replace, error);
    }
};

struct TestSuite
{
    int passed = 0;
    int failed = 0;
    int skipped = 0;

    void run(const std::string& name, const std::function<void()>& test)
    {
        try
        {
            test();
            ++passed;
            std::cout << "PASS " << name << '\n';
        }
        catch (const SkipTest& error)
        {
            ++skipped;
            std::cout << "SKIP " << name << ": " << error.what() << '\n';
        }
        catch (const std::exception& error)
        {
            ++failed;
            std::cerr << "FAIL " << name << ": " << error.what() << '\n';
        }
    }
};
}

int main()
{
    TestSuite suite;
    suite.run("1.1 empty selected directory", []
    {
        TempDirectory fixture;
        const auto result = FileScanner{}.scan(fixture.path);
        check(result.complete(), "Empty scan must be complete.");
        check(result.files == 0 && result.folders == 0 && result.totalBytes == 0 &&
              result.entries.empty(), "Selected root must not count as a child.");
    });
    suite.run("1.1 nonexistent root rejected", []
    {
        TempDirectory fixture;
        const auto result = FileScanner{}.scan(fixture.path / "missing");
        check(!result.rootValid && !result.complete() && !result.issues.empty(),
              "Missing root accepted.");
    });
    suite.run("1.1 file root rejected", []
    {
        TempDirectory fixture;
        writeBytes(fixture.path / "ordinary.txt", 3);
        check(!FileScanner{}.scan(fixture.path / "ordinary.txt").rootValid, "File accepted as root.");
    });
    suite.run("1.1 empty API path rejected", []
    {
        check(!FileScanner{}.scan(fs::path{}).rootValid, "Empty API path accepted.");
    });
    suite.run("1.2 nested counts and relative paths", []
    {
        TempDirectory fixture;
        writeBytes(fixture.path / "first.bin", 500000);
        writeBytes(fixture.path / "nested" / "second.bin", 1000000);
        fs::create_directory(fixture.path / "empty");
        const auto result = FileScanner{}.scan(fixture.path);
        check(result.complete() && result.files == 2 && result.folders == 2 &&
              result.totalBytes == 1500000, "Nested totals differ.");
        check(entry(result, "nested/second.bin").sizeBytes == 1000000, "Nested metadata missing.");
        for (const auto& record : result.entries)
        {
            check(record.relativePath.is_relative(), "Absolute record path leaked.");
        }
    });
    suite.run("1.2 deep traversal without recursive calls", []
    {
        TempDirectory fixture;
        auto path = fixture.path;
        for (int i = 0; i < 12; ++i) { path /= "d"; fs::create_directory(path); }
        writeBytes(path / "last.txt", 7);
        const auto result = FileScanner{}.scan(fixture.path);
        check(result.complete() && result.folders == 12 && result.files == 1 &&
              result.totalBytes == 7, "Deep scan incorrect.");
    });
    suite.run("1.3 root and nested git directories excluded", []
    {
        TempDirectory fixture;
        writeBytes(fixture.path / ".git" / "ignored", 900);
        writeBytes(fixture.path / "nested" / ".git" / "ignored", 800);
        writeBytes(fixture.path / "visible.txt", 5);
        const auto result = FileScanner{}.scan(fixture.path);
        check(result.complete() && result.files == 1 && result.folders == 1 &&
              result.totalBytes == 5 && result.excludedEntries == 2, "Git exclusion incorrect.");
    });
    suite.run("1.3 git pointer file and git-selected roots protected", []
    {
        TempDirectory fixture;
        writeBytes(fixture.path / ".git", 10);
        check(FileScanner{}.scan(fixture.path).files == 0, "Git pointer file counted.");
        fs::remove(fixture.path / ".git");
        fs::create_directories(fixture.path / ".git" / "objects");
        check(!FileScanner{}.scan(fixture.path / ".git").rootValid, "Git root accepted.");
        check(!FileScanner{}.scan(fixture.path / ".git" / "objects").rootValid,
              "Git descendant accepted.");
    });
    suite.run("1.3 external file symlink skipped", []
    {
        TempDirectory fixture;
        fs::create_directory(fixture.path / "selected");
        writeBytes(fixture.path / "outside.txt", 99);
        makeLink(fixture.path / "outside.txt", fixture.path / "selected" / "link.txt");
        const auto result = FileScanner{}.scan(fixture.path / "selected");
        check(result.complete() && result.files == 0 && result.totalBytes == 0 &&
              result.skippedLinks == 1, "File symlink counted or followed.");
        check(entry(result, "link.txt").kind == EntryKind::SkippedLink, "Wrong link record type.");
    });
    suite.run("1.3 directory cycle and external directory links skipped", []
    {
        TempDirectory fixture;
        fs::create_directory(fixture.path / "selected");
        writeBytes(fixture.path / "outside" / "secret.txt", 99);
        makeLink(fixture.path / "outside", fixture.path / "selected" / "outside-link", true);
        makeLink(fixture.path / "selected", fixture.path / "selected" / "cycle", true);
        const auto result = FileScanner{}.scan(fixture.path / "selected");
        check(result.complete() && result.files == 0 && result.folders == 0 &&
              result.skippedLinks == 2, "Directory links traversed or counted.");
    });
    suite.run("1.3 broken link skipped", []
    {
        TempDirectory fixture;
        makeLink(fixture.path / "missing", fixture.path / "broken");
        const auto result = FileScanner{}.scan(fixture.path);
        check(result.complete() && result.files == 0 && result.skippedLinks == 1,
              "Broken link caused a failed scan or counted as a file.");
    });
    suite.run("1.3 symbolic-link root rejected including trailing separator", []
    {
        TempDirectory fixture;
        fs::create_directory(fixture.path / "ordinary");
        makeLink(fixture.path / "ordinary", fixture.path / "link", true);
        check(!FileScanner{}.scan(fixture.path / "link").rootValid, "Link root accepted.");
        check(!FileScanner{}.scan((fixture.path / "link") / "").rootValid,
              "Trailing separator bypassed root policy.");
    });
    suite.run("1.3 special FIFO entry skipped", []
    {
#ifdef _WIN32
        throw SkipTest("POSIX FIFO fixture is not available on Windows.");
#else
        TempDirectory fixture;
        if (mkfifo((fixture.path / "pipe").c_str(), 0600) != 0)
        {
            throw std::runtime_error("Could not create owned FIFO fixture.");
        }
        const auto result = FileScanner{}.scan(fixture.path);
        check(result.complete() && result.files == 0 && result.skippedOther == 1,
              "FIFO was counted as a regular file.");
#endif
    });
    suite.run("1.3 real permission denial reported and sibling scanning continues", []
    {
        TempDirectory fixture;
        const auto locked = fixture.path / "locked";
        writeBytes(locked / "hidden.txt", 10);
        writeBytes(fixture.path / "visible.txt", 4);
        PermissionRestore restore{locked, fs::status(locked).permissions()};
        std::error_code error;
        fs::permissions(locked, fs::perms::none, fs::perm_options::replace, error);
        if (error) { throw SkipTest("Permission fixture unavailable: " + error.message()); }
        fs::directory_iterator probe(locked, error);
        if (!error) { throw SkipTest("This account can still read the permission-restricted directory."); }
        if (error != std::errc::permission_denied)
        {
            throw SkipTest("Fixture did not produce permission_denied: " + error.message());
        }
        const auto result = FileScanner{}.scan(fixture.path);
        check(result.rootValid && !result.complete() && hasIssue(result, "open directory") &&
              result.files == 1 && result.totalBytes == 4, "Permission failure hidden or sibling lost.");
    });
    suite.run("1.4 extension, type, size and modification records", []
    {
        TempDirectory fixture;
        writeBytes(fixture.path / "notes.TXT", 13);
        const auto result = FileScanner{}.scan(fixture.path);
        const auto& record = entry(result, "notes.TXT");
        check(result.complete() && record.kind == EntryKind::RegularFile &&
              record.extension == ".TXT" && record.sizeBytes == 13 &&
              record.modifiedTime.has_value(), "Incomplete metadata record.");
    });
    suite.run("1.4 zero-byte and extensionless files preserved", []
    {
        TempDirectory fixture;
        writeBytes(fixture.path / "README", 0);
        const auto result = FileScanner{}.scan(fixture.path);
        const auto& record = entry(result, "README");
        check(result.complete() && result.files == 1 && record.extension.empty() &&
              record.sizeBytes.has_value() && *record.sizeBytes == 0, "Zero size treated as missing.");
    });
    suite.run("1.4 path with spaces and Unicode filename", []
    {
        TempDirectory fixture;
        const auto selected = fixture.path / "folder with spaces";
        const auto name = fs::u8path(u8"\u09ac\u09be\u0982\u09b2\u09be.txt");
        writeBytes(selected / name, 8);
        const auto result = FileScanner{}.scan(selected);
        check(result.complete() && result.files == 1 &&
              entry(result, name.generic_u8string()).sizeBytes == 8, "Unicode/spaced path changed.");
    });
    suite.run("1.4 injected size error excludes sentinel and retains good file", []
    {
        TempDirectory fixture;
        writeBytes(fixture.path / "bad.bin", 500000);
        writeBytes(fixture.path / "good.bin", 1000000);
        FileScanner scanner([](const fs::path& path, std::error_code& error)
        {
            if (path.filename() == "bad.bin")
            {
                error = std::make_error_code(std::errc::permission_denied);
                return std::numeric_limits<std::uintmax_t>::max();
            }
            return fs::file_size(path, error);
        });
        const auto result = scanner.scan(fixture.path);
        check(!result.complete() && result.files == 2 && result.totalBytes == 1000000 &&
              !entry(result, "bad.bin").sizeBytes.has_value() &&
              hasIssue(result, "read file size"), "Failed size included or error hidden.");
    });
    suite.run("1.4 disappearing file reports metadata issue", []
    {
        TempDirectory fixture;
        writeBytes(fixture.path / "vanishing.txt", 3);
        FileScanner scanner([](const fs::path& path, std::error_code& error)
        {
            const auto size = fs::file_size(path, error);
            std::error_code removalError;
            fs::remove(path, removalError); // Only this owned disposable fixture.
            return size;
        });
        const auto result = scanner.scan(fixture.path);
        check(!result.complete() && hasIssue(result, "read modification time"),
              "Disappearing file was reported as fully measured.");
    });
    suite.run("1.4 total-byte overflow is detected", []
    {
        TempDirectory fixture;
        writeBytes(fixture.path / "a", 1);
        writeBytes(fixture.path / "b", 1);
        FileScanner scanner([](const fs::path&, std::error_code& error)
        {
            error.clear();
            return std::numeric_limits<std::uintmax_t>::max();
        });
        const auto result = scanner.scan(fixture.path);
        check(!result.complete() && result.totalBytes == std::numeric_limits<std::uintmax_t>::max() &&
              hasIssue(result, "sum file sizes"), "Overflow wrapped or went unreported.");
    });
    suite.run("1.5 repeatable sorted records and fresh state", []
    {
        TempDirectory fixture;
        for (int i = 99; i >= 0; --i)
        {
            writeBytes(fixture.path / ("file-" + std::to_string(i) + ".bin"), static_cast<std::size_t>(i));
        }
        FileScanner scanner;
        const auto first = scanner.scan(fixture.path);
        const auto second = scanner.scan(fixture.path);
        check(first.complete() && second.complete() && second.files == 100 &&
              second.totalBytes == 4950 && first.entries.size() == second.entries.size(),
              "Repeated scan accumulates stale state.");
        for (std::size_t i = 0; i < first.entries.size(); ++i)
        {
            check(first.entries[i].relativePath == second.entries[i].relativePath,
                  "Order changed between scans.");
            if (i != 0)
            {
                check(first.entries[i-1].relativePath.generic_u8string() <
                      first.entries[i].relativePath.generic_u8string(), "Records not sorted.");
            }
        }
    });
    suite.run("1.5 integrated CLI exact decimal summary", []
    {
        TempDirectory fixture;
        writeBytes(fixture.path / "first.bin", 500000);
        writeBytes(fixture.path / "nested" / "second.bin", 1000000);
        fs::create_directory(fixture.path / "empty");
        std::istringstream input(fixture.path.u8string() + "\n");
        std::ostringstream output;
        check(runApplication(input, output) == 0, "Successful CLI scan returned failure.");
        for (const std::string text : {"Total Files: 2", "Total Folders: 2",
             "Total Size: 1500000 bytes", "Total Size (KB): 1500.00", "Total Size (MB): 1.50"})
        {
            check(output.str().find(text) != std::string::npos, "Missing CLI output: " + text);
        }
        check(output.str().find("Warning:") == std::string::npos, "False warning on healthy scan.");
    });
    suite.run("1.5 integrated CLI invalid input and EOF", []
    {
        TempDirectory fixture;
        std::istringstream missing((fixture.path / "missing").u8string() + "\n");
        std::ostringstream missingOutput;
        check(runApplication(missing, missingOutput) == 1 &&
              missingOutput.str().find("Invalid folder path!") != std::string::npos,
              "CLI missing-root status incorrect.");
        std::istringstream emptyInput;
        std::ostringstream eofOutput;
        check(runApplication(emptyInput, eofOutput) == 1 &&
              eofOutput.str().find("Total Files:") == std::string::npos,
              "EOF unexpectedly scanned the working directory.");
    });
    suite.run("1.5 integrated CLI partial scan warning and exit status", []
    {
        TempDirectory fixture;
        writeBytes(fixture.path / "unmeasured.bin", 10);
        FileScanner scanner([](const fs::path&, std::error_code& error)
        {
            error = std::make_error_code(std::errc::permission_denied);
            return std::numeric_limits<std::uintmax_t>::max();
        });
        std::istringstream input(fixture.path.u8string() + "\n");
        std::ostringstream output;
        check(runApplication(input, output, scanner) == 2, "Partial scan exit status is not 2.");
        check(output.str().find("Warning: scan incomplete") != std::string::npos &&
              output.str().find("Total Size: 0 bytes") != std::string::npos &&
              output.str().find("[size unavailable]") != std::string::npos,
              "Partial scan message/totals incorrect.");
    });
    suite.run("1.5 blank interactive line selects controlled current directory", []
    {
        TempDirectory fixture;
        writeBytes(fixture.path / "one.txt", 2);
        struct RestoreDirectory
        {
            fs::path previous = fs::current_path();
            ~RestoreDirectory() { fs::current_path(previous); }
        } restore;
        fs::current_path(fixture.path);
        std::istringstream input("\n");
        std::ostringstream output;
        check(runApplication(input, output) == 0 &&
              output.str().find("Total Size: 2 bytes") != std::string::npos,
              "Blank interactive input lost existing default behavior.");
    });

    std::cout << "\nRESULT: " << suite.passed << " passed, " << suite.failed
              << " failed, " << suite.skipped << " skipped\n";
    return suite.failed == 0 ? 0 : 1;
}
