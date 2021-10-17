module;

#include "global.h"

export module mf;

import gba;

export class Mf : public Gba
{
public:
    explicit Mf(std::filesystem::path filepath);
};
