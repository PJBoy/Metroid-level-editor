#pragma once

class Os
{
public:
    virtual ~Os() = default;

    virtual int eventLoop() = 0;
};
