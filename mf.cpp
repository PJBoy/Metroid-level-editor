#include "mf.h"

#include "global.h"

Mf::Mf(std::filesystem::path filepath)
try
    : Gba(filepath)
{}
LOG_RETHROW
