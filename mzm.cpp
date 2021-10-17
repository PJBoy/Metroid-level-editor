#include "global.h"

import mzm;

Mzm::Mzm(std::filesystem::path filepath)
try
    : Gba(filepath)
{
    throw std::runtime_error("Invalid Metroid Zero Mission ROM"s);
}
LOG_RETHROW
