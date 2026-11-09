#include "menu.hpp"

#include <filesystem>
#include <istream>
#include <ostream>
#include <system_error>

namespace folderflow
{
namespace
{
namespace fs = std::filesystem;

std::string trim(const std::string& text)
{
    const auto first = text.find_first_not_of(" \t\r");
    if (first == std::string::npos)
        return {};
    return text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
}

bool readPath(std::istream& input, std::ostream& output, const char* prompt,
              bool directory, std::string& path, const std::string& defaultPath = {})
{
    output << prompt << std::flush;
    if (!std::getline(input, path))
        return false;

    path = trim(path);
    if (path.size() >= 2 && path.front() == '"' && path.back() == '"')
        path = path.substr(1, path.size() - 2);
    else if (path.empty())
        path = defaultPath;

    std::error_code error;
    if (!path.empty() && path.find('\0') == std::string::npos)
    {
        try
        {
            const auto status = fs::status(fs::u8path(path), error);
            if (!error && (directory ? fs::is_directory(status) : fs::is_regular_file(status)))
                return true;
        }
        catch (const fs::filesystem_error&)
        {
            // Path conversion can fail before the status call is reached.
        }
    }
    output << (directory ? "Enter an existing folder path.\n" : "Enter an existing file path.\n");
    return false;
}

bool confirm(std::istream& input, std::ostream& output, const std::string& folder)
{
    output << "This will move files in: " << folder
           << "\nType YES to proceed, or press Enter to cancel: " << std::flush;
    std::string answer;
    if (!std::getline(input, answer) || trim(answer) != "YES")
    {
        output << "Cancelled.\n";
        return false;
    }
    return true;
}

struct Option
{
    char number;
    const char* label;
    const char* command;
};

constexpr Option options[] = {
    {'1', "Scan a folder", "--scan"},
    {'2', "Analyze storage", "--analyze-storage"},
    {'3', "Search a saved index", "--find-name"},
    {'4', "Preview organization", "--dry-run"},
    {'5', "Organize files", "--organize"},
    {'6', "Undo organization", "--undo"},
    {'7', "Find duplicate files", "--find-duplicates"},
};
}

int runMenu(std::istream& input, std::ostream& output, const MenuCommand& execute,
            bool fileMovesAvailable)
{
    output << "FolderFlow Guardian\n";
    while (input)
    {
        output << '\n';
        const auto available = [fileMovesAvailable](const Option& option)
        {
            return fileMovesAvailable || (option.number != '5' && option.number != '6');
        };
        for (const auto& option : options)
            if (available(option))
                output << "  " << option.number << ". " << option.label << '\n';
        output << "  0. Exit\nChoose a number: " << std::flush;

        std::string choice;
        if (!std::getline(input, choice))
            break;
        choice = trim(choice);
        if (choice == "0")
        {
            output << "Goodbye.\n";
            return 0;
        }

        const Option* selected = nullptr;
        for (const auto& option : options)
            if (choice.size() == 1 && choice.front() == option.number && available(option))
                selected = &option;
        if (!selected)
        {
            output << "Invalid choice. Enter a number shown in the menu.\n";
            continue;
        }

        const char number = selected->number;
        std::string path;
        std::string extra;
        if (number == '3')
        {
            if (!readPath(input, output, "Saved index file: ", false, path))
                continue;
            output << "Part of the filename to find: " << std::flush;
            if (!std::getline(input, extra))
                continue;
            extra = trim(extra);
            if (extra.empty() || extra.find('\0') != std::string::npos)
            {
                output << "Enter a filename or part of one.\n";
                continue;
            }
        }
        else
        {
            if (!readPath(input, output, "Folder path: ", true, path))
                continue;
            if ((number == '4' || number == '5') &&
                !readPath(input, output,
                          "Rules file [Enter for config/extension-rules.txt]: ",
                          false, extra, "config/extension-rules.txt"))
                continue;
            if ((number == '5' || number == '6') && !confirm(input, output, path))
                continue;
        }

        std::vector<std::string> arguments{selected->command, path};
        if (!extra.empty())
            arguments.push_back(extra);
        const int status = execute(arguments);
        if (status != 0)
            output << "Operation ended with status " << status << ". See the details above.\n";
    }

    if (input.bad() || !input.eof())
    {
        output << "Could not read input. Closing menu.\n";
        return 1;
    }
    output << "End of input. Goodbye.\n";
    return 0;
}
}
