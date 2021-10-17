module;

#include "global.h"

export module mzm;

import gba;

export class Mzm : public Gba
{
public:
    explicit Mzm(std::filesystem::path filepath);
};
