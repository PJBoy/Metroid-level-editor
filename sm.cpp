#include "sm.h"

#include "global.h"

Sm::Sm(std::filesystem::path filepath)
try
    : Rom(filepath)
{}
LOG_RETHROW
