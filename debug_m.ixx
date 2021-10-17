export module debug_m;

import typedefs_m;

import std.core;

using namespace std::string_literals;

export class DebugFile : public std::ofstream
{
    // MSVC bug: making this static variable inline causes the following const inline statics to be empty strings
    static std::filesystem::path dataDirectory;

public:
    const inline static std::string
        error{"debug_error.txt"s},
        warning{"debug_warning.txt"s},
        info{"debug_info.txt"s};

    explicit DebugFile(std::filesystem::path filename) noexcept;

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
