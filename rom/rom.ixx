module;

#include "../global.h"

export module rom;

import array2d;
import data_reader;

export namespace rom
{
using Address = data_reader::Address;

struct Bgr15
{
    uint16_t colour;
};

struct Abgr16 // 1-bit alpha
{
    uint16_t colour; // MSb = 1 for transparent
};
}

using namespace rom;

export class Rom
{
public:
    virtual ~Rom() = default;

    virtual Array2d<Abgr16> drawRoom(Address address) const = 0;
};

export std::unique_ptr<Rom> loadRom(std::filesystem::path filepath);
export bool isValidRom(const std::filesystem::path& filepath) noexcept;
