#pragma once

#include "global.h"

class Os
{
public:
    virtual ~Os() = default;

    virtual int eventLoop() = 0;
};
