#include "rom.h"

#include "global.h"

Rom::Reader::Reader(std::filesystem::path filepath)
try
    : f(filepath, std::ios::binary)
{
    f.exceptions(std::ios::badbit | std::ios::failbit);
}
LOG_RETHROW


Rom::Rom(std::filesystem::path filepath)
try
    : filepath(filepath)
{}
LOG_RETHROW


Rom::Reader Rom::makeReader() const
try
{
    return Rom::Reader(filepath);
}
LOG_RETHROW


bool Rom::verifyRom(std::filesystem::path filepath) noexcept
try
{
    loadRom(filepath);
    return true;
}
catch (const std::exception&)
{
    return false;
}

std::unique_ptr<Rom> Rom::loadRom(std::filesystem::path filepath)
try
{
    throw std::runtime_error("Not a valid ROM"s);
}
LOG_RETHROW
