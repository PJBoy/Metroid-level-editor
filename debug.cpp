#include "debug.h"
#include "global.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <unordered_set>
#include <utility>

struct FilepathHash
{
    size_t operator()(const std::filesystem::path& filepath) const
    {
        return hash_value(weakly_canonical(filepath));
    }
};

DebugFile::DebugFile(std::filesystem::path filename)
try
{
    const std::filesystem::path filepath(dataDirectory / filename);

    // If first use of file, clear it, otherwise insert spaces
    static std::unordered_set<std::filesystem::path, FilepathHash> initialised;
    if (initialised.insert(filepath).second)
        open(std::move(filepath), trunc);
    else
        open(std::move(filepath), app | ate);

    const std::time_t time(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    *this << std::put_time(std::gmtime(&time), "%c") << " - "s;
}
catch (const std::exception&)
{
    // There's nothing I can do to handle this exception; but catch it anyways, because this is a non-fatal error
}

void DebugFile::init(std::filesystem::path dataDirectory_in)
{
    dataDirectory = dataDirectory_in;
}

void DebugFile::writeImage(const halfword* data, word width, word height)
{
    byte bmpHeader[]
    {
        0x42, 0x4D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x28, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    *((word*)(&bmpHeader[2])) = width * height + sizeof(bmpHeader);
    *((word*)(&bmpHeader[0x12])) = width;
    *((word*)(&bmpHeader[0x16])) = height;
    *((word*)(&bmpHeader[0x22])) = width * height;
    write((char*)(bmpHeader), sizeof(bmpHeader));

    const n_t padding((4 - width * 3 % 4) % 4);
    for (index_t y(height); y; --y)
    {
        for (index_t x(0); x < width; ++x)
        {
            const halfword bgr15(data[y*width + x]);
            const byte bgr24[]
            {
                byte((bgr15 >> 10 & 0x1F) * 8),
                byte((bgr15 >> 5  & 0x1F) * 8),
                byte((bgr15       & 0x1F) * 8)
            };
            write((char*)(bgr24), sizeof(bgr24));
        }

        for (index_t i(padding); i; --i)
            put(0);
    }
}
