#include "../global.h"

import rom;

import sm;

std::unique_ptr<Rom> loadRom(std::filesystem::path filepath)
{
    std::unique_ptr<Sm> p_sm = Sm::loadRom(filepath);
    if (p_sm)
        return p_sm;

    return {};
}

bool isValidRom(const std::filesystem::path& filepath) noexcept
{
    return Sm::isValidRom(filepath);
}
