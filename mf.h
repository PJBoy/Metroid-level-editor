#pragma once

#include "gba.h"

#include "global.h"

class Mf : public Gba
{
public:
    explicit Mf(std::filesystem::path filepath);
};
