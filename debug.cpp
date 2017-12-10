#include "debug.h"
#include "global.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iterator>
#include <utility>

DebugFile::DebugFile(const std::string_view& filepath)
    : std::ofstream(std::data(filepath), app | ate)
{
    // Clear file
    static bool initialised(false);
    if (!std::exchange(initialised, true))
    {
        close();
        open(std::data(filepath), trunc);
    }
    
    exceptions(failbit | badbit);

    if (tellp() != std::streampos())
        *this << "\n\n";

    const std::time_t time(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    *this << std::put_time(std::gmtime(&time), "%c") << '\n';
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
