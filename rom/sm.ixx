module;

#include "../global.h"

export module sm;

export import rom;
import data_reader;

export struct SnesAddress : data_reader::Address
{
    SnesAddress() = default;
    explicit SnesAddress(index_t snesAddress);
    SnesAddress(data_reader::Address address);

    friend SnesAddress operator"" _sm(unsigned long long snesAddress);
};

export namespace sm
{
using byte_t = uint8_t;
using word_t = uint16_t;
using long_t = uint32_t;

using metatileBitmap_t = std::array<std::array<rom::Abgr16, 0x10>, 0x10>;

struct StateCondition
{
    SnesAddress address;
    word_t condition, stateAddress;
    union
    {
        word_t doorAddress;
        byte_t eventIndex, boss;
    };
    
    StateCondition() = default;
    explicit StateCondition(DataReader& reader);
};

struct RoomHeader
{
    SnesAddress address;
    byte_t
        roomIndex, areaIndex,
        mapX, mapY,
        width, height,
        upScroller, downScroller,
        creBitset;

    word_t doorListAddress;
    std::vector<StateCondition> stateConditions;

    RoomHeader() = default;
    explicit RoomHeader(DataReader& reader);

    std::vector<SnesAddress> getStateAddresses() const;
};

struct StateHeader
{
    SnesAddress address;
    long_t levelDataAddress;
    word_t
        fxAddress,
        enemyPopulationAddress,
        enemySetAddress,
        scrollAddress,
        xrayAddress,
        mainAsmAddress,
        plmPopulationAddress,
        libraryBackgroundAddress,
        setupAsmAddress;

    byte_t
        tilesetIndex,
        musicDataIndex, musicTrack,
        layer2ScrollX, layer2ScrollY;
    
    StateHeader() = default;
    explicit StateHeader(DataReader& reader);
};

struct LevelData
{
    SnesAddress address;
    std::vector<word_t> level, background;
    std::vector<byte_t> bts;

    LevelData() = default;
    explicit LevelData(DataReader& r);
};

struct TilesetHeader
{
    SnesAddress address;
    long_t metatilesAddress, tileGfxsAddress, palettesAddress;
    
    TilesetHeader() = default;
    explicit TilesetHeader(DataReader& reader);
};

struct Mode1Tileset;
struct Mode7Tileset;
}

export class Sm : public Rom
{
    static const n_t n_rom = 3ul << 20;
    static const n_t n_tilesets = 0x1D;
    
    std::array<uint8_t, n_rom> rom;
    std::filesystem::path filepath;
    std::map<SnesAddress, sm::RoomHeader> roomHeaders;
    std::map<SnesAddress, sm::StateHeader> stateHeaders;
    std::array<sm::TilesetHeader, n_tilesets> tilesetHeaders;

public:
    static std::unique_ptr<Sm> loadRom(std::filesystem::path filepath);
    static bool isValidRom(const std::filesystem::path& filepath) noexcept;

    Sm() = default;
    explicit Sm(std::filesystem::path filepath);

    Array2d<rom::Abgr16> drawRoom(rom::Address address) const override;

private:
    void readRomFromFile();
    void loadTilesetTable();
    void loadRoom(SnesAddress roomAddress);
    void loadState(SnesAddress stateAddress);
    sm::Mode1Tileset makeMode1Tileset(index_t i_tileset, bool isExtraLarge, bool isCeres) const;
    std::vector<sm::metatileBitmap_t> drawMode1Tileset(index_t i_tileset, bool isExtraLarge, bool isCeres) const;
};
