#pragma once

#include "rom.h"

#include "global.h"

#include <cinttypes>

class Gba : public Rom
{
protected:
    using byte_t     = std::uint8_t;
    using halfword_t = std::uint16_t;
    using word_t     = std::uint32_t;

    class Pointer : public Wrapper<word_t>
    {
    public:
        using Wrapper::Wrapper;
        using Wrapper::operator=;

        inline operator word_t() const
        {
            return v - 0x800'0000;
        }
    };

    explicit Gba(std::filesystem::path filepath);

    friend Pointer operator"" _gba(unsigned long long pointer);
};

inline Gba::Pointer operator"" _gba(unsigned long long pointer)
{
    return Gba::Pointer(Gba::word_t(pointer));
}