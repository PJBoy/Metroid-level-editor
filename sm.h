#pragma once

#include "rom.h"

#include "global.h"

#include <cinttypes>

class Sm : public Rom
{
    using byte_t = std::uint8_t;
    using word_t = std::uint16_t;
    using long_t = std::uint32_t;

    class Pointer : public Wrapper<long_t>
    {
    public:
        using Wrapper::Wrapper;
        using Wrapper::operator=;

        inline operator long_t() const
        {
            return v >> 1 & 0x3F8000 | v & 0x7FFF;
        }
    };

    friend Pointer operator"" _sm(unsigned long long pointer);

public:
    explicit Sm(std::filesystem::path path);
};

inline Sm::Pointer operator"" _sm(unsigned long long pointer)
{
    return Sm::Pointer(Sm::long_t(pointer));
}
