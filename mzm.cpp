#include "mzm.h"

#include "global.h"

Mzm::Mzm(std::filesystem::path filepath)
try
    : Gba(filepath)
{}
LOG_RETHROW
