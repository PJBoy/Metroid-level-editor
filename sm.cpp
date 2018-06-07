#include "sm.h"

#include "global.h"

#include <array>
#include <string_view>

Sm::Sm(std::filesystem::path filepath) try
    : Rom(filepath)
{
    Reader r(makeReader());
    if (std::array title(r.get<char, 0x15>(0xFFC0_sm)); std::string_view(std::data(title), std::size(title)) != "Super Metroid        "s)
        throw std::runtime_error("Invalid Super Metroid ROM (incorrect header title)"s);

    if (byte_t region(r.get<byte_t>(0xFFD9_sm)); region != 0)
        if (region == 2)
            throw std::runtime_error("PAL Super Metroid not supported"s);
        else
            throw std::runtime_error("Invalid Super Metroid ROM (incorrect header region)"s);
}
LOG_RETHROW
