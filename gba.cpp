#include "gba.h"

#include "global.h"

Gba::Gba(std::filesystem::path filepath)
try
    : Rom(filepath)
{}
LOG_RETHROW
