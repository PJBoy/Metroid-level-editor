#pragma once

#include "gba.h"

#include "global.h"

class Mzm : public Gba
{
public:
    explicit Mzm(std::filesystem::path filepath);
};
