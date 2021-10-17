module;

#include "global.h"

export module config;
//import typedefs_m;

using namespace std::literals;
export class Config
{
    const static unsigned maxVersion{0};
    const inline static std::filesystem::path filename{"config.ini"s};
    std::filesystem::path filepath;

public:
    std::vector<std::filesystem::path> recentFiles;

    explicit Config(const std::filesystem::path& dataDirectory);
    
    void save() const;
    void load();
    void addRecentFile(std::filesystem::path filepath);
};
