#pragma once

#include "rom.h"

#include "global.h"

#include <filesystem>
#include <memory>

class Os
{
    std::unique_ptr<Rom> p_rom;

public:
    virtual ~Os() = default;

    virtual void init() = 0;
    virtual int eventLoop() = 0;
    virtual std::filesystem::path getDataDirectory() const = 0;
};
