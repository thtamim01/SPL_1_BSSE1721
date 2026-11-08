#include "rule_config.hpp"

#include <cctype>
#include <fstream>
#include <set>
#include <string>

namespace folderflow
{
namespace
{
std::string trim(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return {};

    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

bool validExtension(const std::string& extension)
{
    if (extension.size() < 2 || extension[0] != '.')
        return false;

    for (std::size_t i = 1; i < extension.size(); ++i)
    {
        const unsigned char ch =
            static_cast<unsigned char>(extension[i]);

        if (!std::isalnum(ch) || ch > 127)
            return false;
    }

    return true;
}

bool validDestination(const std::string& destination)
{
    if (destination.empty() || destination == "." ||
        destination == "..")
        return false;

    for (const unsigned char ch : destination)
    {
        const bool letter =
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= 'a' && ch <= 'z');
        const bool digit = ch >= '0' && ch <= '9';

        if (!letter && !digit && ch != ' ' &&
            ch != '_' && ch != '-')
            return false;
    }

    return true;
}
}

RuleConfigResult loadExtensionRules(
    const std::filesystem::path& configPath)
{
    RuleConfigResult result;
    std::ifstream input(configPath);

    if (!input)
    {
        result.error = "Could not open rule file.";
        return result;
    }

    std::set<std::string> seenExtensions;
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(input, line))
    {
        ++lineNumber;

        // PowerShell's UTF-8 output may include a BOM.
        if (lineNumber == 1 && line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF)
        {
            line.erase(0, 3);
        }

        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;

        const auto separator = line.find('=');
        if (separator == std::string::npos ||
            line.find('=', separator + 1) != std::string::npos)
        {
            result.error = "Line " + std::to_string(lineNumber) +
                           ": expected .extension=Folder";
            result.rules.clear();
            return result;
        }

        std::string extension = trim(line.substr(0, separator));
        const std::string destination =
            trim(line.substr(separator + 1));

        if (!validExtension(extension))
        {
            result.error = "Line " + std::to_string(lineNumber) +
                           ": invalid extension";
            result.rules.clear();
            return result;
        }

        if (!validDestination(destination))
        {
            result.error = "Line " + std::to_string(lineNumber) +
                           ": destination must be one folder name";
            result.rules.clear();
            return result;
        }

        for (char& ch : extension)
        {
            ch = static_cast<char>(
                std::tolower(static_cast<unsigned char>(ch)));
        }

        if (!seenExtensions.insert(extension).second)
        {
            result.error = "Line " + std::to_string(lineNumber) +
                           ": duplicate extension " + extension;
            result.rules.clear();
            return result;
        }

        result.rules.push_back({extension, destination});
    }

    if (input.bad())
    {
        result.rules.clear();
        result.error = "Error while reading rule file.";
    }

    return result;
}
}