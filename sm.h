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
    {};

public:
    explicit Sm(std::filesystem::path path);
};
