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

        operator word_t() const
        {
            return v - 0x800'0000;
        }
    };

    explicit Gba(std::filesystem::path filepath);
};
