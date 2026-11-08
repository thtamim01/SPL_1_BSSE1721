#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace folderflow
{
struct ExtensionRule
{
    std::string extension;
    std::filesystem::path destination;
};

struct RuleConfigResult
{
    std::vector<ExtensionRule> rules;
    std::string error;
};

RuleConfigResult loadExtensionRules(
    const std::filesystem::path& configPath);
}