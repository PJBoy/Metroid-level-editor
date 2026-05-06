#include "../global.h"

import sm;

Sm::Sm(std::filesystem::path filepath)
try
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
        throw std::runtime_error(LOG_INFO "Failed to open ROM");

    file.seekg(0, std::ios::end);
    const n_t fileSize = file.tellg();
    if (fileSize != 3 << 20)
        throw std::runtime_error(LOG_INFO "Incorrect ROM size");

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(std::data(rom)), std::size(rom));
    if (!file)
        throw std::runtime_error(LOG_INFO "Failed to read ROM");
}
LOG_RETHROW

std::unique_ptr<Sm> sm::loadRom(std::filesystem::path filepath)
try
{
    return std::unique_ptr<Sm>(new Sm(std::move(filepath)));
}
LOG_RETHROW

bool sm::isValidRom(const std::filesystem::path& filepath) noexcept
{
    std::ifstream file(filepath, std::ios::binary);
    file.seekg(0, std::ios::end);
    const n_t fileSize = file.tellg();
    if (fileSize != 3 << 20)
        return false;
    
    const std::array<uint8_t, 0x15> expectedTitle{0x53, 0x75, 0x70, 0x65, 0x72, 0x20, 0x4D, 0x65, 0x74, 0x72, 0x6F, 0x69, 0x64, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
    std::array<uint8_t, 0x15> title;
    file.seekg(0x7FC0, std::ios::beg);
    file.read(reinterpret_cast<char*>(std::data(title)), std::size(title));
    if (!file)
        return false;

    if (title != expectedTitle)
        return false;

    return true;
}
