#pragma once

#include "global.h"

#include <filesystem>

class Os
{
public:
    virtual ~Os() = default;

    virtual void init() = 0;
    virtual int eventLoop() = 0;
    virtual std::filesystem::path getDataDirectory() const = 0;
};
