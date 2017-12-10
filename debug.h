#pragma once

#include "global.h"
#include <fstream>
#include <string>
#include <string_view>

class DebugFile : public std::ofstream
{
public:
    DebugFile(const std::string_view& filepath);

    void writeImage(const halfword* data, word width, word height);
};
