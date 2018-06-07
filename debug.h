#pragma once

#include "global.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#define LOG_INFO __FILE__ ":" APPLY(STRINGIFY, __LINE__) " - "s
#define LOG_RETHROW \
    catch (const std::exception& e) \
    { \
        DebugFile(DebugFile::error) << LOG_INFO "Exception thrown: "s << e.what() << '\n'; \
        throw; \
    }

class DebugFile : public std::ofstream
{
    // MSVC bug: making this static variable inline causes the following const inline statics to be empty strings
    static std::filesystem::path dataDirectory;

public:
    const inline static std::string
        error{"debug_error.txt"s},
        warning{"debug_warning.txt"s},
        info{"debug_info.txt"s};

    DebugFile(std::filesystem::path filename) noexcept;

    template<typename T>
    DebugFile& operator<<(const T& v)
    {
        std::cout << v;
        *static_cast<std::ofstream*>(this) << v;
        return *this;
    }

    static void init(std::filesystem::path dataDirectory_in) noexcept;
    void writeImage(const uint16_t* data, uint32_t width, uint32_t height);
};
