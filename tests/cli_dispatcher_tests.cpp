#include "../src/cli_dispatcher.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace
{
namespace fs = std::filesystem;

class Fixture
{
public:
    Fixture()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto temp = fs::temp_directory_path();
        for (int attempt = 0; attempt < 20; ++attempt)
        {
            const auto candidate = temp / ("FolderFlow-dispatch-" + std::to_string(stamp) +
                                           "-" + std::to_string(attempt));
            if (fs::create_directory(candidate))
            {
                root = candidate;
                return;
            }
        }
        throw std::runtime_error("Could not create an owned test directory");
    }

    ~Fixture()
    {
        std::error_code ignored;
        fs::remove_all(root, ignored);
    }

    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;

    fs::path root;
};

void writeFile(const fs::path& path, const char* content)
{
    std::ofstream file(path, std::ios::binary);
    file << content;
    file.close();
    if (!file)
        throw std::runtime_error("Could not write a fixture file");
}

struct Result
{
    int status;
    std::string output;
    std::string errors;
};

Result run(std::initializer_list<std::string> arguments, const std::string& lines = "")
{
    std::vector<const char*> argv{"folderflow"};
    for (const auto& argument : arguments)
        argv.push_back(argument.c_str());

    std::istringstream input(lines);
    std::ostringstream output;
    std::ostringstream errors;
    const int status = folderflow::runCli(static_cast<int>(argv.size()), argv.data(),
                                          input, output, errors);
    return {status, output.str(), errors.str()};
}

void check(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void expectOutput(const Result& result, const char* expected)
{
    if (result.status != 0 || !result.errors.empty() ||
        result.output.find(expected) == std::string::npos)
        throw std::runtime_error(std::string("Expected output: ") + expected +
                                 "\nOutput: " + result.output + "\nErrors: " + result.errors);
}
}

int main()
{
    int passed = 0;
    int failed = 0;
    const auto test = [&](const char* name, auto body)
    {
        try
        {
            body();
            ++passed;
            std::cout << "PASS " << name << '\n';
        }
        catch (const std::exception& error)
        {
            ++failed;
            std::cerr << "FAIL " << name << ": " << error.what() << '\n';
        }
    };

    test("no arguments open the menu and 0 exits", []
    {
        const auto result = run({}, "0\n");
        expectOutput(result, "Choose a number:");
        expectOutput(result, "Goodbye.");
    });
    test("unknown command and invalid argument counts return usage errors", []
    {
        for (const auto& result : {run({"--unknown"}), run({"--dry-run", "."}),
                                  run({"--scan", ".", "."}), run({"--undo"})})
            check(result.status == 1 && result.output.empty() &&
                  result.errors.find("Usage:") == 0, "Expected usage on stderr");
    });
    test("scan supports the path argument and the existing prompt", []
    {
        Fixture fixture;
        writeFile(fixture.root / "report.pdf", "report");
        const auto folder = fixture.root.u8string();
        expectOutput(run({"--scan", folder}), "report.pdf");
        expectOutput(run({"--scan"}, folder + "\n"), "report.pdf");
    });
    test("menu scan and direct scan reach the same command", []
    {
        Fixture fixture;
        writeFile(fixture.root / "menu report.pdf", "report");
        const auto folder = fixture.root.u8string();
        const auto direct = run({"--scan", folder});
        expectOutput(direct, "menu report.pdf");
        const auto menu = run({}, "1\n" + folder + "\n0\n");
        expectOutput(menu, direct.output.c_str());
        expectOutput(menu, "Goodbye.");
    });
    test("storage analysis accepts a top count and rejects invalid counts", []
    {
        Fixture fixture;
        writeFile(fixture.root / "report.pdf", "report");
        const auto folder = fixture.root.u8string();
        expectOutput(run({"--analyze-storage", folder, "1"}), "report.pdf");
        const auto invalid = run({"--analyze-storage", folder, "1x"});
        check(invalid.status == 1 && invalid.output.empty() &&
              invalid.errors.find("Top count must be an integer") == 0,
              "Invalid top count should fail before analysis");
    });
    test("index commands save, load, compare, search and summarize", []
    {
        Fixture fixture;
        const auto folder = fixture.root / "files";
        fs::create_directory(folder);
        writeFile(folder / "report.pdf", "report");
        const auto root = folder.u8string();
        const auto index = (fixture.root / "saved.index").u8string();
        expectOutput(run({"--save-index", root, index}), "Saved 1 file(s).");
        expectOutput(run({"--load-index", index}), "report.pdf");
        expectOutput(run({"--check-index", root, index}), "Changed: 0, Missing: 0");
        expectOutput(run({"--find-name", index, "report"}), "Matches: 1");
        expectOutput(run({"--index-summary", index}), "Total files: 1");
    });
    test("rule checking, evaluation and preview route without moving files", []
    {
        Fixture fixture;
        const auto folder = fixture.root / "files";
        fs::create_directory(folder);
        const auto report = folder / "report.pdf";
        writeFile(report, "report");
        const auto rules = fixture.root / "rules.txt";
        writeFile(rules, ".pdf=Documents\n");
        const auto root = folder.u8string();
        const auto config = rules.u8string();
        expectOutput(run({"--check-rules", config}), "Loaded rules: 1");
        expectOutput(run({"--evaluate-rules", root, config}), "report.pdf -> Documents/report.pdf");
        expectOutput(run({"--dry-run", root, config}), "[DRY RUN: move proposed]");
        check(fs::is_regular_file(report) && !fs::exists(folder / "Documents") &&
              !fs::exists(folder / ".folderflow-undo"), "Preview modified the fixture");
    });
    test("duplicate CLI and menu agree without changing files", []
    {
        Fixture fixture;
        const auto folder = fixture.root / "files";
        fs::create_directory(folder);
        writeFile(folder / "first.bin", "same");
        writeFile(folder / "second.bin", "same");
        const auto root = folder.u8string();
        const auto direct = run({"--find-duplicates", root});
        const auto menu = run({}, "7\n" + root + "\n0\n");
        expectOutput(direct, "Confirmed groups: 1");
        expectOutput(menu, "Confirmed groups: 1");
        check(menu.output.find("Potential reclaimable logical bytes: 4") != std::string::npos &&
              fs::is_regular_file(folder / "first.bin") &&
              fs::is_regular_file(folder / "second.bin") &&
              fs::directory_iterator(folder) != fs::directory_iterator{},
              "Duplicate report modified files");
    });
    test("organize and undo honor platform support", []
    {
        Fixture fixture;
        const auto folder = fixture.root / "files";
        fs::create_directory(folder);
        const auto report = folder / "report.pdf";
        const auto target = folder / "Documents" / "report.pdf";
        writeFile(report, "report");
        const auto rules = fixture.root / "rules.txt";
        writeFile(rules, ".pdf=Documents\n");
        const auto root = folder.u8string();
        const auto organize = run({"--organize", root, rules.u8string()});
#ifdef _WIN32
        expectOutput(organize, "MOVED: report.pdf -> Documents/report.pdf");
        check(!fs::exists(report) && fs::is_regular_file(target), "File was not moved");
        expectOutput(run({"--undo", root}), "RESTORED: Documents/report.pdf -> report.pdf");
        check(!fs::exists(target.parent_path()), "Undo did not remove the empty folder");
#else
        check(organize.status == 1 && organize.errors.find("Windows only") != std::string::npos,
              "Expected the organization platform error");
        const auto undo = run({"--undo", root});
        check(undo.status == 1 && undo.errors.find("Windows only") != std::string::npos,
              "Expected the undo platform error");
#endif
        check(fs::is_regular_file(report) && !fs::exists(target), "Unexpected file locations");
    });

    std::cout << "CLI RESULT: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
