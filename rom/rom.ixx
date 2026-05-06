module;

#include "../global.h"

export module rom;

export class Rom
{
public:
    virtual ~Rom() = default;
};

export std::unique_ptr<Rom> loadRom(std::filesystem::path filepath);
export bool isValidRom(const std::filesystem::path& filepath) noexcept;
