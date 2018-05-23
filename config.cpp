#include "config.h"

#include "global.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <regex>
#include <stdexcept>
#include <sstream>

void Config::init(const std::filesystem::path& dataDirectory)
try
{
    filepath = dataDirectory / filename;
}
LOG_RETHROW

void Config::save() const
try
{
    std::ofstream out(filepath);
    out.exceptions(std::ios::badbit | std::ios::failbit);
    out << "Version: "s << maxVersion << '\n';
    for (const std::filesystem::path& file : recentFiles)
        out << "File: "s << file << '\n';
}
LOG_RETHROW

void Config::load()
try
{
    const std::regex
        regex_comment(R"(\s*(?:#.*)?)"s, std::regex::optimize),
        regex_version(R"(\s*version:\s*(\d+)\s*)"s, std::regex::optimize | std::regex::icase),
        regex_file(R"(\s*file:\s*(.+)\s*)"s, std::regex::optimize | std::regex::icase);

    std::ifstream in(filepath);
    std::optional<unsigned> opt_version;
    for (std::string line; std::getline(in, line);)
    {
        std::smatch match;

        // Comment
        if (std::regex_match(line, match, regex_comment))
            continue;

        // Version (this should be the first value)
        if (std::regex_match(line, match, regex_version))
        {
            unsigned long version;
            try
            {
                version = std::stoul(match[1]);
            }
            catch (const std::out_of_range& e)
            {
                throw std::runtime_error(LOG_INFO "Version in config file ("s + std::string(match[1]) + ") is greater than supported ("s + std::to_string(maxVersion) + "): "s + e.what());
            }
            catch (const std::invalid_argument& e)
            {
                throw std::runtime_error(LOG_INFO "Invalid version in config file ("s + std::string(match[1]) + "): "s + e.what());
            }

            if (opt_version)
            {
                DebugFile(DebugFile::warning) << LOG_INFO "Version specified more than once, ignoring additional versions\n"s;
                continue;
            }
            
            if (version > maxVersion)
                throw std::runtime_error(LOG_INFO "Version in config "s + std::to_string(version) + "file is greater than supported ("s + std::to_string(maxVersion) + ")"s);

            opt_version = version;
            continue;
        }

        // If we got this far without a version, that's an error
        if (!opt_version)
            throw std::runtime_error(LOG_INFO "Version not specified first in config file"s);

        // Recent files
        if (std::regex_match(line, match, regex_file))
        {
            std::filesystem::path path;
            std::istringstream in_filepath{std::string(match[1])};
            in_filepath >> path;
            recentFiles.push_back(std::move(path));
            continue;
        }
        
        // Unrecognised values; ignore them
        DebugFile(DebugFile::warning) << LOG_INFO "Unknown config key\n"s;
    }
}
LOG_RETHROW

void Config::addRecentFile(std::filesystem::path recentFilepath)
try
{
    if (auto&& it(std::find(std::begin(recentFiles), std::end(recentFiles), recentFilepath)); it != std::end(recentFiles))
        recentFiles.erase(it);

    recentFiles.push_back(recentFilepath);
}
LOG_RETHROW
