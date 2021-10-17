module;

#include "global.h"

export module os;

import config;
import rom;

export class Os
{
protected:
    std::unique_ptr<Rom> p_rom;
    Config* p_config{};

public:
    explicit Os() = default;
    Os(const Os&) = delete;
    auto operator=(Os) = delete;
    virtual ~Os() = default;

    virtual int eventLoop() = 0;
    virtual std::filesystem::path getDataDirectory() const = 0;
    virtual void error(const std::string& errorText) const = 0;

    virtual void init(Config& config)
    {
        p_config = &config;
    }
};
