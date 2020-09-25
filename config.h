#pragma once

#include "global.h"

#include <filesystem>
#include <vector>

class Config
{
    const static unsigned maxVersion{0};
    const inline static std::filesystem::path filename{"config.ini"s};
    std::filesystem::path filepath;

public:
    std::vector<std::filesystem::path> recentFiles;

    void save() const;
    void load();
    void init(const std::filesystem::path& dataDirectory);
    void addRecentFile(std::filesystem::path filepath);
};
