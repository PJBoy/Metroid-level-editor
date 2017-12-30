#pragma once

#include "global.h"

#include <filesystem>
#include <iosfwd>
#include <vector>

class Config
{
    const static unsigned maxVersion{0};
    const inline static std::experimental::filesystem::path filename{"config.ini"s};
    std::experimental::filesystem::path filepath;

public:
    std::vector<std::experimental::filesystem::path> recentFiles;

    void save() const;
    void load();
    void init(const std::experimental::filesystem::path& dataDirectory);
    void addRecentFile(std::experimental::filesystem::path filepath);
};
