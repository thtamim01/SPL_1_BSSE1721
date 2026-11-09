#include "../src/quarantine.hpp"
#include "../src/scanner.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace
{
namespace fs = std::filesystem;
struct Fixture
{
    fs::path root;
    Fixture()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int i = 0; i < 30; ++i)
        {
            const auto candidate = fs::temp_directory_path() /
                ("FolderFlow-quarantine-test-" + std::to_string(stamp) + "-" + std::to_string(i));
            if (fs::create_directory(candidate))
            {
                root = candidate;
                return;
            }
        }
        throw std::runtime_error("Could not create owned fixture");
    }
    ~Fixture()
    {
        std::error_code ec;
        fs::remove_all(root, ec);
    }
};
void check(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}
void write(const fs::path& file, const std::string& text)
{
    std::ofstream output(file, std::ios::binary);
    output << text;
    output.close();
    check(static_cast<bool>(output), "Fixture write failed");
}
std::string read(const fs::path& file)
{
    std::ifstream input(file, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), {});
}
struct Result
{
    int status;
    std::string output, errors;
};
Result quarantine(const fs::path& root, const fs::path& file)
{
    std::ostringstream out, err;
    const int status = folderflow::quarantineFile(root, file, out, err);
    return {status, out.str(), err.str()};
}
Result list(const fs::path& root)
{
    std::ostringstream out, err;
    const int status = folderflow::listQuarantine(root, out, err);
    return {status, out.str(), err.str()};
}
Result restore(const fs::path& root, const std::string& id)
{
    std::ostringstream out, err;
    const int status = folderflow::restoreQuarantine(root, id, out, err);
    return {status, out.str(), err.str()};
}
std::string identifier(const Result& result)
{
    const auto start = result.output.find("ID: ");
    check(start != std::string::npos, "Recovery ID missing");
    return result.output.substr(start + 4, 32);
}
}

int main()
{
    int passed = 0, failed = 0, skipped = 0;
    const auto test = [&](const char* name, auto body)
    {
        try { body(); ++passed; std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& e) { ++failed; std::cerr << "FAIL " << name << ": " << e.what() << '\n'; }
    };

    test("quarantine persists metadata and restores exact contents", []
    {
        Fixture f;
        fs::create_directory(f.root / "nested");
        write(f.root / "nested" / "report.bin", "private bytes");
        const auto moved = quarantine(f.root, "nested/report.bin");
        check(moved.status == 0 && !fs::exists(f.root / "nested/report.bin"), "Quarantine failed");
        const auto id = identifier(moved);
        const auto held = f.root / ".folderflow-quarantine" / id / "payload";
        check(read(held) == "private bytes" && read(f.root / ".folderflow-quarantine" / id / "record.txt")
              .find("nested/report.bin") != std::string::npos, "Recovery data missing");
        const auto found = list(f.root);
        check(found.status == 0 && found.output.find(id + " QUARANTINED") != std::string::npos,
              "Persistent listing failed");
        check(restore(f.root, id).status == 0 && read(f.root / "nested/report.bin") == "private bytes",
              "Restore did not preserve bytes");
        check(list(f.root).output.find(id + " RESTORED") != std::string::npos,
              "Restore state was not recorded");
    });
    test("two files receive different recovery IDs", []
    {
        Fixture f;
        write(f.root / "one", "1"); write(f.root / "two", "2");
        const auto first = quarantine(f.root, "one");
        const auto second = quarantine(f.root, "two");
        check(first.status == 0 && second.status == 0 &&
              identifier(first) != identifier(second), "Identifiers collided");
    });
    test("zero byte file round trips", []
    {
        Fixture f;
        write(f.root / "empty", "");
        const auto id = identifier(quarantine(f.root, "empty"));
        check(restore(f.root, id).status == 0 && fs::file_size(f.root / "empty") == 0,
              "Empty payload failed to restore");
    });
    test("occupied restore destination never overwritten", []
    {
        Fixture f;
        write(f.root / "file", "original");
        const auto id = identifier(quarantine(f.root, "file"));
        write(f.root / "file", "new contents");
        const auto result = restore(f.root, id);
        check(result.status == 1 && read(f.root / "file") == "new contents" &&
              read(f.root / ".folderflow-quarantine" / id / "payload") == "original",
              "Conflict overwrote data");
    });
    test("missing payload remains an explicit recovery issue", []
    {
        Fixture f;
        write(f.root / "file", "data");
        const auto id = identifier(quarantine(f.root, "file"));
        fs::rename(f.root / ".folderflow-quarantine" / id / "payload", f.root / "removed-for-test");
        check(restore(f.root, id).status != 0 && list(f.root).status == 2,
              "Missing content reported as recovered");
    });
    test("damaged record is rejected without moving payload", []
    {
        Fixture f;
        write(f.root / "file", "data");
        const auto id = identifier(quarantine(f.root, "file"));
        const auto record = f.root / ".folderflow-quarantine" / id / "record.txt";
        write(record, "corrupted");
        check(restore(f.root, id).status != 0 && list(f.root).status == 2 &&
              read(f.root / ".folderflow-quarantine" / id / "payload") == "data",
              "Damaged record lost file");
    });
    test("pending record after move can be restored", []
    {
        Fixture f;
        write(f.root / "file", "data");
        const auto id = identifier(quarantine(f.root, "file"));
        const auto record = f.root / ".folderflow-quarantine" / id / "record.txt";
        auto text = read(record);
        text.resize(text.size() - std::string("STORED\n").size());
        write(record, text);
        check(list(f.root).status == 2 && list(f.root).output.find(" PENDING ") != std::string::npos,
              "Pending move was not reported explicitly");
        check(restore(f.root, id).status == 0 && read(f.root / "file") == "data",
              "Interrupted move could not recover");
    });
    test("unsafe paths and missing files rejected", []
    {
        Fixture f;
        write(f.root / "file", "data");
        check(quarantine(f.root, "../file").status == 1 &&
              quarantine(f.root, ".folderflow-undo/journal").status == 1 &&
              quarantine(f.root, ".folderflow-quarantine/file").status == 1 &&
              quarantine(f.root, "absent").status == 1 &&
              fs::exists(f.root / "file"), "Unsafe path changed file");
    });
    test("scanner excludes internal quarantine tree", []
    {
        Fixture f;
        write(f.root / "file", "data");
        const auto id = identifier(quarantine(f.root, "file"));
        const auto scan = folderflow::FileScanner{}.scan(f.root);
        check(scan.rootValid && scan.entries.empty() &&
              !folderflow::FileScanner{}.scan(f.root / ".folderflow-quarantine").rootValid &&
              fs::exists(f.root / ".folderflow-quarantine" / id / "payload"),
              "Quarantine data reached scanner");
    });
    {
    Fixture f;
    write(f.root / "file", "data");
    std::error_code linkError;
    fs::create_symlink(f.root / "file", f.root / "link", linkError);
    if (linkError)
    {
        ++skipped;
        std::cout << "SKIP linked file fixture: " << linkError.message() << '\n';
    }
    else test("linked file is rejected", [&]
    {
        check(quarantine(f.root, "link").status == 1 && fs::exists(f.root / "file"),
              "Symbolic link followed");
        fs::create_directory(f.root / "real");
        write(f.root / "real" / "nested", "data");
        std::error_code directoryError;
        fs::create_directory_symlink(f.root / "real", f.root / "folder-link", directoryError);
        if (!directoryError)
            check(quarantine(f.root, "folder-link/nested").status == 1 &&
                  fs::exists(f.root / "real" / "nested"), "Linked parent followed");
        const auto id = identifier(quarantine(f.root, "file"));
        std::error_code destinationError;
        fs::create_symlink(f.root / "missing-target", f.root / "file", destinationError);
        if (!destinationError)
            check(restore(f.root, id).status == 1 &&
                  fs::is_symlink(fs::symlink_status(f.root / "file")) &&
                  fs::exists(f.root / ".folderflow-quarantine" / id / "payload"),
                  "Dangling link destination was overwritten");
    });
    }
    std::cout << "QUARANTINE RESULT: " << passed << " passed, " << failed
              << " failed, " << skipped << " skipped\n";
    return failed == 0 ? 0 : 1;
}
