#pragma once

#include "byte_cast_iterator.h"
#include "matrix.h"
#include "rom.h"

#include "global.h"

#include <array>
#include <cinttypes>

class Sm : public Rom
{
public:
    using byte_t = std::uint8_t;
    using word_t = std::uint16_t;
    using long_t = std::uint32_t;


private:
    class Pointer
    {
        long_t v{};

    public:
    // Operator overloads
    #if 1
    #define operators(op) \
        constexpr Pointer operator##op(Pointer rhs) \
        { \
            return Pointer(v op long_t(rhs.v), true); \
        } \
    \
        template<typename U> \
        constexpr friend Pointer operator##op(Pointer lhs, U rhs) \
        { \
            return Pointer(lhs.v op long_t(rhs), true); \
        } \
    \
        template<typename U> \
        constexpr friend Pointer operator##op(U lhs, Pointer rhs) \
        { \
            return Pointer(long_t(lhs) op rhs.v, true); \
        } \
    \
        template<typename U> \
        constexpr Pointer& operator##op##=(U rhs) \
        { \
            return v op##= rhs, *this; \
        } \

        operators(+)
        operators(-)
        operators(|)
        operators(&)
    #undef operators

    #define operator(op) \
        constexpr bool operator##op(Pointer rhs) \
        { \
            return v op rhs.v; \
        } \

        operator(<)
        operator(>)
        operator(<=)
        operator(>=)
        operator(==)
        operator(!=)
    #undef operator

        constexpr Pointer& operator++()
        {
            return ++v, *this;
        }

        constexpr Pointer& operator--()
        {
            return --v, *this;
        }

        constexpr Pointer operator++(int)
        {
            return ++*this - 1;
        }

        constexpr Pointer operator--(int)
        {
            return --*this + 1;
        }
    #endif

        Pointer() = default;
        
        constexpr explicit Pointer(long_t pointer, bool pc = false)
        {
            if (pc)
                v = pointer;
            else if (pointer >= 0x80'0000 && pointer & 0x8000)
                v = pointer >> 1 & 0x3F'8000 | pointer & 0x7FFF;
            else
                throw std::out_of_range("Invalid SNES pointer"s);
        }

        constexpr long_t pc() const noexcept
        {
            return v;
        }

        constexpr long_t snes() const noexcept
        {
            return v << 1 & 0xFF0000 | v & 0xFFFF | 0x808000;
        }
    };

    constexpr friend Pointer operator"" _sm(unsigned long long pointer);

    class Reader : public Rom::Reader
    {
    public:
        using Rom::Reader::Reader;
        using Rom::Reader::get;
        using Rom::Reader::peek;
        using Rom::Reader::seek;

        Reader(std::filesystem::path filepath, Pointer address)
        try
            : Reader(filepath, address.pc())
        {}
        LOG_RETHROW

        void seek(Pointer address)
        try
        {
            Rom::Reader::seek(address.pc());
        }
        LOG_RETHROW

        template<typename T, n_t size = sizeof(T)>
        T get(Pointer address)
        try
        {
            return Rom::Reader::get<T, size>(address.pc());
        }
        LOG_RETHROW

        template<typename T, n_t size = sizeof(T)>
        void get(Pointer address, T* dest, n_t n)
        try
        {
            return Rom::Reader::get<T, size>(address.pc(), dest, n);
        }
        LOG_RETHROW

        template<n_t n, typename T, n_t size = sizeof(T)>
        std::array<T, n> get(Pointer address)
        try
        {
            return Rom::Reader::get<n, T, size>(address.pc());
        }
        LOG_RETHROW

        template<typename T, n_t size = sizeof(T)>
        T peek(Pointer address)
        try
        {
            return Rom::Reader::peek<T, size>(address.pc());
        }
        LOG_RETHROW

        template<n_t n, typename T, n_t size = sizeof(T)>
        std::array<T, n> peek(Pointer address)
        try
        {
            return Rom::Reader::peek<n, T, size>(address.pc());
        }
        LOG_RETHROW

        long_t getLong()
        try
        {
            return get<long_t, 3>();
        }
        LOG_RETHROW

        template<n_t n>
        std::array<long_t, n> getLongs()
        try
        {
            return get<n, long_t, 3>();
        }
        LOG_RETHROW

        template<typename T>
        long_t getLong(T address)
        try
        {
            return get<long_t, 3>(address);
        }
        LOG_RETHROW

        template<typename T, n_t n>
        std::array<long_t, n> getLongs(T address)
        try
        {
            return get<n, long_t, 3>(address);
        }
        LOG_RETHROW

        long_t peekLong()
        try
        {
            return peek<long_t, 3>();
        }
        LOG_RETHROW

        template<n_t n>
        std::array<long_t, n> peekLongs()
        try
        {
            return peek<n, long_t, 3>();
        }
        LOG_RETHROW

        template<typename T>
        long_t peekLong(T address)
        try
        {
            return peek<long_t, 3>(address);
        }
        LOG_RETHROW

        template<typename T, n_t n>
        std::array<long_t, n> peekLongs(T address)
        try
        {
            return peek<n, long_t, 3>(address);
        }
        LOG_RETHROW
    };

    Reader makeReader(Pointer address = {}) const;

    struct RoomHeader
    {
        struct EventHeader
        {
            struct Base
            {
                word_t p_id, p_state;

                Base() = default;
                virtual ~Base() = default;

                Base(Reader& r)
                try
                {
                    p_id = r.get<word_t>();
                    p_state = r.get<word_t>();
                }
                LOG_RETHROW

                virtual n_t size() const noexcept
                {
                    return 4;
                }
            };

            struct Door : Base
            {
                word_t p_door;

                Door(Reader& r)
                try
                {
                    p_id = r.get<word_t>();
                    p_door = r.get<word_t>();
                    p_state = r.get<word_t>();
                }
                LOG_RETHROW

                virtual n_t size() const noexcept override
                {
                    return 6;
                }
            };

            struct MainAreaBoss : Base
            {
                using Base::Base;
            };

            struct Event : Base
            {
                byte_t i_event;

                Event(Reader& r)
                try
                {
                    p_id = r.get<word_t>();
                    i_event = r.get<byte_t>();
                    p_state = r.get<word_t>();
                }
                LOG_RETHROW

                virtual n_t size() const noexcept override
                {
                    return 5;
                }
            };

            struct Boss : Base
            {
                byte_t boss;

                Boss(Reader& r)
                try
                {
                    p_id = r.get<word_t>();
                    boss = r.get<byte_t>();
                    p_state = r.get<word_t>();
                }
                LOG_RETHROW

                virtual n_t size() const noexcept override
                {
                    return 5;
                }
            };

            struct Morph : Base
            {
                using Base::Base;
            };

            struct MorphMissiles : Base
            {
                using Base::Base;
            };

            struct PowerBombs : Base
            {
                using Base::Base;
            };

            struct SpeedBooster : Base
            {
                using Base::Base;
            };

            EventHeader() = delete;

            static std::unique_ptr<EventHeader::Base> loadEventHeader(Reader& r);
        };

        struct StateHeader
        {
            long_t p_levelData;
            word_t
                p_fx,
                p_enemyPopulation,
                p_enemySet,
                p_scroll,
                p_xray,
                p_mainAsm,
                p_plm,
                p_bg,
                p_setupAsm;

            byte_t
                i_tileset,
                i_musicData, i_musicTrack,
                layer2ScrollX, layer2ScrollY;

            StateHeader() = default;
            StateHeader(Reader& r);
        };

        byte_t
            roomIndex, areaIndex,
            mapX, mapY,
            width, height,
            upScroller, downScroller,
            creBitset;

        word_t p_doorList;
        std::vector<std::unique_ptr<EventHeader::Base>> events;
        StateHeader defaultState;

        RoomHeader() = default;
        RoomHeader(Reader& r);

        inline n_t size() const noexcept
        {
            n_t ret(9 + 2 + 2); 
            for (const std::unique_ptr<EventHeader::Base>& p_event : events)
                ret += p_event->size();

            return ret;
        }
    };

    struct LevelData
    {
        n_t n_x{}, n_y{};

        Matrix<word_t> layer1, layer2;
        Matrix<byte_t> bts;

        LevelData() = default;
        LevelData(n_t n_y, n_t n_x, bool isCustomLayer2, Reader& r);
    };

    static std::vector<byte_t> decompress(n_t maxSize, Reader& r);
    static n_t decompress(byte_t* dest, n_t capacity, Reader& r);

    template<typename T>
    static n_t decompress(T* dest, n_t capacity, Reader& r)
    try
    {
        return decompress(reinterpret_cast<byte_t*>(dest), capacity * sizeof(T), r);
    }
    LOG_RETHROW

    template<typename T, n_t n>
    static n_t decompress(T(&dest)[n], Reader& r)
    try
    {
        return decompress(dest, n, r);
    }
    LOG_RETHROW
        
    template<typename T, n_t n>
    static n_t decompress(std::array<T, n>& dest, Reader& r)
    try
    {
        return decompress(dest, n, r);
    }
    LOG_RETHROW


    using metatile_t = std::array<word_t, 4>;
    using tile_t = std::array<byte_t, 0x20>;
    using palette_t = std::array<word_t, 0x10>;

    metatile_t metatiles[0x100 + 0x300]; // CRE is 100h metatiles
    tile_t tiles[0x280 + 0x180]; // CRE is 180h tiles
    palette_t bgPalettes[8];
    Cairo::RefPtr<Cairo::ImageSurface> metatileSurfaces[0x400];
    Cairo::RefPtr<Cairo::ImageSurface> p_layer1, p_layer2;
    Cairo::RefPtr<Cairo::ImageSurface> p_level;
    std::vector<std::pair<Pointer, RoomHeader>> knownRoomHeaders;
    LevelData levelData;

    void decompressTileset(index_t i_tileset);
    void createMetatileSurface(index_t i_metatiles);
    void createMetatileSurfaces();
    Cairo::RefPtr<Cairo::ImageSurface> createLayerSurface(const Matrix<word_t>& layer) const;
    void loadTileset(index_t i_tileset);
    
    void findRoomHeaders();

public:
    explicit Sm(std::filesystem::path path);

    virtual void drawLevelView(Cairo::RefPtr<Cairo::Surface> p_surface) const override;
    virtual std::vector<RoomList> getRoomList() const override;
    virtual void loadLevelData(std::vector<long> ids);
};

constexpr Sm::Pointer operator"" _sm(unsigned long long pointer)
{
    return Sm::Pointer(Sm::long_t(pointer));
}
