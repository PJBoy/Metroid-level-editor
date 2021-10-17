module;

#include "global.h"

export module gba;

import rom;

export class Gba : public Rom
{
public:
    using byte_t     = std::uint8_t;
    using halfword_t = std::uint16_t;
    using word_t     = std::uint32_t;


protected:
#if 1
    struct Pointer
    {
        word_t v{};
    };
#endif

    explicit Gba(std::filesystem::path filepath);

    constexpr friend Pointer operator"" _gba(unsigned long long pointer);
};

constexpr Gba::Pointer operator"" _gba(unsigned long long pointer)
{
    return {Gba::word_t(pointer)};
}