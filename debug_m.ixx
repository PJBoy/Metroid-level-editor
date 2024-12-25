export module debug;

import typedefs;

import std;

using namespace std::string_literals;

export class DebugFile : public std::ofstream
{
    // MSVC bug: making this static variable inline causes the following const inline statics to be empty strings
    static std::filesystem::path dataDirectory;

public:
    const inline static std::filesystem::path
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
};
