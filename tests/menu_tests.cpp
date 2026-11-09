#include "../src/menu.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace
{
namespace fs = std::filesystem;
using Arguments = std::vector<std::string>;

struct Session
{
    std::ostringstream output;
    std::vector<Arguments> calls;

    int run(std::istream& input, bool moves = true, int commandStatus = 0)
    {
        return folderflow::runMenu(input, output, [&](const Arguments& args)
        {
            calls.push_back(args);
            return commandStatus;
        }, moves);
    }

    int run(const std::string& lines, bool moves = true, int commandStatus = 0)
    {
        std::istringstream input(lines);
        return run(input, moves, commandStatus);
    }

    bool says(const std::string& text) const
    {
        return output.str().find(text) != std::string::npos;
    }
};

void check(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
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

    // The handler is a recorder. No test command changes files.
    const auto folder = fs::current_path().u8string();
    const std::string file = "config/extension-rules.txt";
    test("scan, analyze, search and preview dispatch", [&]
    {
        Session session;
        const auto input = " 1 \r\n\"" + folder + "\"\r\n2\n" + folder +
                           "\n3\n" + file + "\npart name\n4\n" + folder + "\n\n0\n";
        check(session.run(input) == 0, "Navigation failed");
        check(session.calls == std::vector<Arguments>{{"--scan", folder},
              {"--analyze-storage", folder}, {"--find-name", file, "part name"},
              {"--dry-run", folder, file}}, "Wrong arguments");
    });
    test("invalid menu choices return to menu", []
    {
        Session session;
        check(session.run("junk\n10\n\n0\n") == 0 && session.calls.empty() &&
              session.says("Invalid choice") && session.says("Goodbye"), "Invalid choice dispatched");
    });
    test("duplicate choice accepts a folder and works without move support", [&]
    {
        Session session;
        check(session.run("7\n" + folder + "\n0\n", false) == 0 &&
              session.calls == std::vector<Arguments>{{"--find-duplicates", folder}} &&
              session.says("Find duplicate files"), "Duplicate menu route missing");
    });
    test("blank and wrong-kind paths are rejected", [&]
    {
        Session session;
        check(session.run("1\n\n2\n" + file + "\n3\n" + folder + "\n0\n") == 0 &&
              session.calls.empty() && session.says("existing folder") &&
              session.says("existing file"), "Bad path dispatched");
    });
    test("missing path is rejected", [&]
    {
        Session session;
        const auto missing = (fs::u8path(file) / "cannot-be-a-child").u8string();
        check(session.run("1\n" + missing + "\n0\n") == 0 && session.calls.empty(),
              "Missing path dispatched");
    });
    test("empty search is rejected", [&]
    {
        Session session;
        check(session.run("3\n" + file + "\n   \n0\n") == 0 && session.calls.empty() &&
              session.says("Enter a filename"), "Empty search dispatched");
    });
    test("organize and undo cancel on any answer except YES", [&]
    {
        Session session;
        check(session.run("5\n" + folder + "\n\nyes\n6\n" + folder + "\n\n0\n") == 0 &&
              session.calls.empty() && session.says("Cancelled"), "Unconfirmed move dispatched");
    });
    test("explicit confirmation dispatches organization and undo", [&]
    {
        Session session;
        check(session.run("5\n" + folder + "\n" + file + "\nYES\n6\n" +
              folder + "\nYES\n0\n") == 0 &&
              session.calls == std::vector<Arguments>{{"--organize", folder, file},
                  {"--undo", folder}}, "Confirmed move did not dispatch");
    });
    test("EOF at confirmation cancels the move", [&]
    {
        Session session;
        check(session.run("5\n" + folder + "\n\n") == 0 && session.calls.empty() &&
              session.says("Cancelled") && session.says("End of input"), "EOF moved files");
    });
    test("unsupported moves are hidden and rejected", []
    {
        Session session;
        check(session.run("5\n6\n0\n", false) == 0 && session.calls.empty() &&
              !session.says("Organize files") && !session.says("Undo organization"),
              "Unsupported action was available");
    });
    test("explicit exit and initial EOF", []
    {
        Session exit;
        Session eof;
        check(exit.run("0\n") == 0 && exit.calls.empty() && exit.says("Goodbye") &&
              eof.run("") == 0 && eof.calls.empty() && eof.says("End of input"), "Exit failed");
    });
    test("stream failure returns an error", []
    {
        Session session;
        std::istringstream broken;
        broken.setstate(std::ios::badbit);
        check(session.run(broken) == 1 && session.calls.empty() &&
              session.says("Could not read input"), "Input error lost");
    });
    test("command failure returns to the menu", [&]
    {
        Session session;
        check(session.run("2\n" + folder + "\n0\n", true, 2) == 0 &&
              session.calls.size() == 1 && session.says("status 2") && session.says("Goodbye"),
              "Command status lost");
    });
    std::cout << "MENU RESULT: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
