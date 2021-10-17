#include "global.h"

import mf;

Mf::Mf(std::filesystem::path filepath)
try
    : Gba(filepath)
{
    throw std::runtime_error("Invalid Metroid Fusion ROM"s);
}
LOG_RETHROW
