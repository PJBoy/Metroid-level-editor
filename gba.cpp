#include "global.h"

import gba;

Gba::Gba(std::filesystem::path filepath)
try
    : Rom(filepath)
{}
LOG_RETHROW
