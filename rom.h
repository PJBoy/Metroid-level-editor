#pragma once

#include "global.h"

#include <filesystem>
#include <memory>

class Rom
{
protected:
    class Reader
    {
        std::ifstream f;

    public:
        explicit Reader(std::filesystem::path filepath);

        template<typename T>
        T get(index_t address)
        try
        {
            T ret;
            f.seekg(address);
            f.read((char*)(&ret), sizeof(ret));
            return ret;
        }
        LOG_RETHROW
    };

    std::filesystem::path filepath;

    explicit Rom(std::filesystem::path filepath);

    Reader makeReader() const;

public:
    static bool verifyRom(std::filesystem::path filepath) noexcept;
    static std::unique_ptr<Rom> loadRom(std::filesystem::path filepath);

    virtual ~Rom() = default;
};
