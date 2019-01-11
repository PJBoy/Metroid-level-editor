#pragma once

#include "global.h"

#include <cairomm/cairomm.h>

#include <filesystem>
#include <memory>

class Rom
{
protected:
    class Reader
    {
        std::ifstream f;

    public:
        explicit Reader(std::filesystem::path filepath, index_t address = 0);

        void seek(index_t address)
        try
        {
            f.seekg(address);
        }
        LOG_RETHROW

        template<typename T, n_t size = sizeof(T)>
        T get()
        try
        {
            static_assert(std::is_integral_v<T>);
            static_assert(size <= sizeof(T));

            T ret{};
            for (index_t i(0); i < size; ++i)
                ret |= static_cast<unsigned char>(f.get()) << i * 8u;

            return ret;
        }
        LOG_RETHROW

        template<typename T, n_t size = sizeof(T)>
        T get(index_t address)
        try
        {
            seek(address);
            return get<T, size>();
        }
        LOG_RETHROW

        template<typename T, n_t size = sizeof(T)>
        void get(T* dest, n_t n)
        try
        {
            static_assert(std::is_integral_v<T>);
            static_assert(size <= sizeof(T));

            if constexpr (size == 1)
                f.read(reinterpret_cast<char*>(dest), n);
            else
                while (n --> 0)
                    *dest++ = get<T, size>();
        }
        LOG_RETHROW

        template<typename T, n_t size = sizeof(T)>
        void get(index_t address, T* dest, n_t n)
        try
        {
            seek(address);
            get<T, size>(dest, n);
        }
        LOG_RETHROW

        template<n_t n, typename T, n_t size = sizeof(T)>
        std::array<T, n> get()
        try
        {
            static_assert(std::is_integral_v<T>);
            static_assert(size <= sizeof(T));

            std::array<T, n> ret{};
            get<T, size>(std::data(ret), n);
            return ret;
        }
        LOG_RETHROW

        template<n_t n, typename T, n_t size = sizeof(T)>
        std::array<T, n> get(index_t address)
        try
        {
            seek(address);
            return get<n, T, size>();
        }
        LOG_RETHROW

        template<typename T, n_t size = sizeof(T)>
        T peek()
        try
        {
            std::streampos pos(f.tellg());
            T ret(get<T, size>());
            f.seekg(pos);
            return ret;
        }
        LOG_RETHROW

        template<typename T, n_t size = sizeof(T)>
        T peek(index_t address)
        try
        {
            std::streampos pos(f.tellg());
            T ret(get<T, size>(address));
            f.seekg(pos);
            return ret;
        }
        LOG_RETHROW

        template<n_t n, typename T, n_t size = sizeof(T)>
        std::array<T, n> peek()
        try
        {
            std::streampos pos(f.tellg());
            T ret(get<n, T, size>());
            f.seekg(pos);
            return ret;
        }
        LOG_RETHROW

        template<n_t n, typename T, n_t size = sizeof(T)>
        std::array<T, n> peek(index_t address)
        try
        {
            std::streampos pos(f.tellg());
            T ret(get<n, T, size>(address));
            f.seekg(pos);
            return ret;
        }
        LOG_RETHROW
    };

    std::filesystem::path filepath;

    explicit Rom(std::filesystem::path filepath);

    Reader makeReader(index_t address = 0) const;
    std::ifstream makeIfstream() const;

public:
    struct RoomList
    {
        long id;
        std::string name;
        std::vector<RoomList> subrooms;
    };

    struct Dimensions
    {
        unsigned blockSize;
        n_t n_y, n_x;
    };

    static bool verifyRom(std::filesystem::path filepath) noexcept;
    static std::unique_ptr<Rom> loadRom(std::filesystem::path filepath);

    virtual ~Rom() = default;

    virtual void drawLevelView(Cairo::RefPtr<Cairo::Surface> p_surface, unsigned x, unsigned y) const;
    virtual Dimensions getLevelViewDimensions() const;
    virtual std::vector<RoomList> getRoomList() const;
    virtual void loadLevelData(std::vector<long> ids);
};
