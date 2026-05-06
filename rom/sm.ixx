module;

#include "../global.h"

export module sm;

export import rom;
import data_reader;

struct SnesAddress : data_reader::Address
{
    SnesAddress() = default;
    explicit SnesAddress(index_t snesAddress)
    {
        if (snesAddress < 0x80'0000 || !(snesAddress & 0x8000))
            throw std::runtime_error("Invalid SNES address");

        address = snesAddress >> 1 & 0x3F'8000 | snesAddress & 0x7FFF;
    }
};

export class Sm : public Rom
{
    std::array<uint8_t, 3 << 20> rom;

public:
    Sm() = default;
    explicit Sm(std::filesystem::path filepath);

};

export namespace sm
{
    std::unique_ptr<Sm> loadRom(std::filesystem::path filepath);
    bool isValidRom(const std::filesystem::path& filepath) noexcept;
}
