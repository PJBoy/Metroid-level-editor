#include "../global.h"

import sm;

import data_reader;

using namespace sm;


constexpr static long_t hex2snes(long_t snesAddress)
{
    return snesAddress << 1 & 0xFF0000 | snesAddress & 0xFFFF | 0x80'8000;
}

static std::vector<byte_t> decompress(DataReader& reader, n_t n_maxDecompressed)
try
{
    std::vector<byte_t> decompressed(n_maxDecompressed);
    index_t i_decompressed{};
    for (;;)
    {
        byte_t byte = reader.readInt<1>();

        // Compressed packets have the format:
        //     tttsssss          ; Short packet (t != 7)
        //     111tttss ssssssss ; Long packet (t != 7 or s < 300h)
        //     11111111          ; Terminator
        // where:
        //     s: Size of decompressed data - 1
        //     t: Type
        // Possibly followed by a 1 or 2 byte argument depending on the type

        if (byte == 0xFF)
            break;

        n_t size = 1;
        unsigned type = byte >> 5;
        if (type != 7)
            size += byte & 0x1F;
        else
        {
            type = byte >> 2 & 7;
            size += (byte & 3) << 8 | reader.readInt<1>();
        }

        if (i_decompressed + size > n_maxDecompressed)
            throw std::out_of_range(LOG_INFO "Decompressed data exceeds size of buffer"s);

        auto it_decompressed = std::begin(decompressed) + i_decompressed;
        
        auto copyForward = [&](index_t i_source)
        {
            // Note that std::copy/memcpy with the destination range beginning within the source range is UB,
            // and std::copy_backward/memmove would be incorrect (needs forward copying semantics)
            for (index_t i{}; i < size; ++i)
                it_decompressed[i] = decompressed[i_source + i];
        };
        
        auto copyForwardInverted = [&](index_t i_source)
        {
            for (index_t i{}; i < size; ++i)
                it_decompressed[i] = ~decompressed[i_source + i];
        };

        switch (type)
        {
        // Direct copy
        case 0:
        {
            std::ranges::copy(reader.readBytes(size), it_decompressed);
            break;
        }

        // Byte fill
        case 1:
        {
            std::fill_n(it_decompressed, size, reader.readInt<1>());
            break;
        }

        // Word fill
        case 2:
        {
            // Odd sizes are allowed, the LSB is copied in this case
            std::array<byte_t, 2> filler{reader.readInt<1>(), reader.readInt<1>()};
            for (index_t i{}; i < size; ++i)
                it_decompressed[i] = filler[i % 2];

            break;
        }

        // Incrementing fill
        case 3:
        {
            std::iota(it_decompressed, it_decompressed + size, reader.readInt<1>());
            break;
        }

        // Dictionary copy
        case 4:
        {
            word_t offset = reader.readInt<2>();
            if (offset + size > n_maxDecompressed)
                throw std::out_of_range(LOG_INFO "Dictionary copy is trying to read out of bounds"s);
            
            copyForward(offset);

            break;
        }

        // Inverted dictionary copy
        case 5:
        {
            word_t offset = reader.readInt<2>();
            if (offset + size > n_maxDecompressed)
                throw std::out_of_range(LOG_INFO "Inverted dictionary copy is trying to read out of bounds"s);

            copyForwardInverted(offset);

            break;
        }

        // Sliding dictionary copy
        case 6:
        {
            byte_t offset = reader.readInt<1>();
            if (offset > i_decompressed)
                throw std::out_of_range(LOG_INFO "Sliding dictionary copy is trying to read out of bounds"s);

            copyForward(i_decompressed - offset);

            break;
        }

        // Inverted sliding dictionary copy
        case 7:
        {
            byte_t offset = reader.readInt<1>();
            if (offset > i_decompressed)
                throw std::out_of_range(LOG_INFO "Inverted sliding dictionary copy is trying to read out of bounds"s);

            copyForwardInverted(i_decompressed - offset);

            break;
        }
        }

        i_decompressed += size;
    }

    decompressed.resize(i_decompressed);
    return decompressed;
}
LOG_RETHROW


SnesAddress::SnesAddress(index_t snesAddress)
{
    if (snesAddress < 0x80'0000 || !(snesAddress & 0x8000))
        throw std::runtime_error("Invalid SNES address");

    address = snesAddress >> 1 & 0x3F'8000 | snesAddress & 0x7FFF;
}

SnesAddress operator"" _sm(unsigned long long snesAddress)
{
    return SnesAddress(snesAddress);
}

SnesAddress::SnesAddress(data_reader::Address address)
    : data_reader::Address(std::move(address))
{}


StateCondition::StateCondition(DataReader& reader)
try
{
    address = reader.tell();
    condition = reader.readInt<2>();
    switch (condition)
    {
    case 0xE5EB:
        doorAddress = reader.readInt<2>();
        break;

    case 0xE612:
        eventIndex = reader.readInt<1>();
        break;

    case 0xE629:
        boss = reader.readInt<1>();
        break;
    }

    if (condition != 0xE5E6)
        stateAddress = reader.readInt<2>();
    else
        stateAddress = word_t(hex2snes(long_t(reader.tell())));
}
LOG_RETHROW


RoomHeader::RoomHeader(DataReader& reader)
try
{
    address = reader.tell();
    reader.readIntsTo<1>(roomIndex, areaIndex, mapX, mapY, width, height, upScroller, downScroller, creBitset);
    doorListAddress = reader.readInt<2>();
    do
        stateConditions.push_back(StateCondition(reader));
    while (stateConditions.back().condition != 0xE5E6);
}
LOG_RETHROW

std::vector<SnesAddress> RoomHeader::getStateAddresses() const
try
{
    std::vector<SnesAddress> stateAddresses;
    for (const StateCondition& condition : stateConditions)
        stateAddresses.push_back(SnesAddress(0x8F'0000 | condition.stateAddress));

    return stateAddresses;
}
LOG_RETHROW


StateHeader::StateHeader(DataReader& reader)
try
{
    address = reader.tell();
    levelDataAddress = reader.readInt<3>();
    reader.readIntsTo<1>(tilesetIndex, musicDataIndex, musicTrack);
    reader.readIntsTo<2>(fxAddress, enemyPopulationAddress, enemySetAddress);
    reader.readIntsTo<1>(layer2ScrollX, layer2ScrollY);
    reader.readIntsTo<2>(scrollAddress, xrayAddress, mainAsmAddress, plmPopulationAddress, libraryBackgroundAddress, setupAsmAddress);
}
LOG_RETHROW


LevelData::LevelData(DataReader& compressedReader)
try
{
    address = compressedReader.tell();

    std::vector<byte_t> decompressed = decompress(compressedReader, 0xFA02);
    DataReader reader(decompressed);
    if (reader.peekInt<1>({0}) % 2 != 0)
        throw std::runtime_error(LOG_INFO "Odd level data size");

    const n_t n = reader.readInt<2>() / 2;
    const n_t n_decompressed = std::size(decompressed);
    bool hasBackground;
    if (n_decompressed == n * 3 + 2)
        hasBackground = false;
    else if (n_decompressed == n * 5 + 2)
        hasBackground = true;
    else
        throw std::runtime_error(LOG_INFO "Bad level data size");

    level.resize(n);
    for (index_t i{}; i < n; ++i)
        level[i] = reader.readInt<2>();

    bts.resize(n);
    for (index_t i{}; i < n; ++i)
        bts[i] = reader.readInt<1>();

    if (!hasBackground)
        return;
    
    background.resize(n);
    for (index_t i{}; i < n; ++i)
        background[i] = reader.readInt<2>();
}
LOG_RETHROW


TilesetHeader::TilesetHeader(DataReader& reader)
try
{
    address = reader.tell();
    reader.readIntsTo<3>(metatilesAddress, tileGfxsAddress, palettesAddress);
}
LOG_RETHROW


std::unique_ptr<Sm> Sm::loadRom(std::filesystem::path filepath)
try
{
    return std::unique_ptr<Sm>(new Sm(std::move(filepath)));
}
LOG_RETHROW

bool Sm::isValidRom(const std::filesystem::path& filepath) noexcept
{
    std::ifstream file(filepath, std::ios::binary);
    file.seekg(0, std::ios::end);
    const n_t fileSize = file.tellg();
    if (fileSize != n_rom)
        return false;
    
    const std::array<uint8_t, 0x15> expectedTitle{0x53, 0x75, 0x70, 0x65, 0x72, 0x20, 0x4D, 0x65, 0x74, 0x72, 0x6F, 0x69, 0x64, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
    std::array<uint8_t, 0x15> title;
    file.seekg(0x7FC0, std::ios::beg);
    file.read(reinterpret_cast<char*>(std::data(title)), std::size(title));
    if (!file)
        return false;

    if (title != expectedTitle)
        return false;

    return true;
}

Sm::Sm(std::filesystem::path filepath)
try
    : filepath(std::move(filepath))
{
    readRomFromFile();
    loadTilesetTable();

    // Temporary
    loadRoom(0x8F'D78F_sm);
}
LOG_RETHROW

struct sm::MetatileBitmap
{
    std::array<std::array<rom::Abgr16, 8>, 8> data;
    bool hasPriority;
};

Array2d<rom::Abgr16> Sm::drawRoom(rom::Address address) const
try
{
    const RoomHeader& roomHeader = roomHeaders.at(address);
    const StateHeader& stateHeader = stateHeaders.at(roomHeader.getStateAddresses().back());

    DataReader reader(rom);
    reader.seek(SnesAddress(stateHeader.levelDataAddress));
    const LevelData levelData(reader);

    const bool isExtraLarge = roomHeader.creBitset & 4;
    const bool isCeres = roomHeader.areaIndex == 6;
    const std::vector<metatileBitmaps_t> tileset = drawMode1Tileset(stateHeader.tilesetIndex, isExtraLarge, isCeres);

    Array2d<rom::Abgr16> bitmap({.n_y = roomHeader.height * 0x100u, .n_x = roomHeader.width * 0x100u});
    std::fill_n(std::data(bitmap), std::size(bitmap), rom::Abgr16{0x8000});

    const n_t n_y_blocks = roomHeader.height * 0x10, n_x_blocks = roomHeader.width * 0x10;
    for (index_t y_block{}; y_block < n_y_blocks; ++y_block)
        for (index_t x_block{}; x_block < n_x_blocks; ++x_block)
        {
            const bool hasBackground = !levelData.background.empty();
            const index_t i_block = y_block * n_x_blocks + x_block;
            const word_t levelBlock = levelData.level[i_block];
            const word_t backgroundBlock = !hasBackground ? 0 : levelData.background[i_block];
            
            const index_t i_levelMetatile = levelBlock & 0x3FF;
            const index_t i_backgroundMetatile = backgroundBlock & 0x3FF;
            const bool isLevelBlockFlipped_x = levelBlock >> 0xA & 1;
            const bool isLevelBlockFlipped_y = levelBlock >> 0xB & 1;
            const bool isBackgroundBlockFlipped_x = backgroundBlock >> 0xA & 1;
            const bool isBackgroundBlockFlipped_y = backgroundBlock >> 0xB & 1;
            
            if (i_levelMetatile > std::size(tileset))
                throw std::out_of_range(LOG_INFO "Out of bounds level metatile index");
            
            if (i_backgroundMetatile > std::size(tileset))
                throw std::out_of_range(LOG_INFO "Out of bounds background metatile index");
            
            for (index_t i_y_tile{}; i_y_tile < 2; ++i_y_tile)
                for (index_t i_x_tile{}; i_x_tile < 2; ++i_x_tile)
                {
                    auto drawTile = [&](const MetatileBitmap& metatile, bool isBlockFlipped_x, bool isBlockFlipped_y)
                    {
                        for (index_t y{}; y < 8; ++y)
                            for (index_t x{}; x < 8; ++x)
                            {
                                index_t
                                    y_bitmap = y_block * 0x10 + i_y_tile * 8,
                                    x_bitmap = x_block * 0x10 + i_x_tile * 8;

                                y_bitmap += !isBlockFlipped_y ? y : 7 - y;
                                x_bitmap += !isBlockFlipped_x ? x : 7 - x;

                                if (!(metatile.data[y][x].colour & 0x8000) || bitmap[y_bitmap][x_bitmap].colour & 0x8000)
                                    bitmap[y_bitmap][x_bitmap] = metatile.data[y][x];
                            }
                    };
                    
                    const index_t
                        i_x_levelTile = !isLevelBlockFlipped_x ? i_x_tile : 1 - i_x_tile,
                        i_y_levelTile = !isLevelBlockFlipped_y ? i_y_tile : 1 - i_y_tile,
                        i_x_backgroundTile = !isBackgroundBlockFlipped_x ? i_x_tile : 1 - i_x_tile,
                        i_y_backgroundTile = !isBackgroundBlockFlipped_y ? i_y_tile : 1 - i_y_tile;

                    const MetatileBitmap& levelMetatile = tileset[i_levelMetatile][i_y_levelTile][i_x_levelTile];
                    const MetatileBitmap& backgroundMetatile = tileset[i_backgroundMetatile][i_y_backgroundTile][i_x_backgroundTile];

                    // drawTile(y_block * n_x_blocks + x_block & 0x3FF)
                    if (hasBackground && backgroundMetatile.hasPriority && !levelMetatile.hasPriority)
                    {
                        drawTile(levelMetatile, isLevelBlockFlipped_x, isLevelBlockFlipped_y);
                        drawTile(backgroundMetatile, isBackgroundBlockFlipped_x, isBackgroundBlockFlipped_y);
                    }
                    else
                    {
                        drawTile(backgroundMetatile, isBackgroundBlockFlipped_x, isBackgroundBlockFlipped_y);
                        drawTile(levelMetatile, isLevelBlockFlipped_x, isLevelBlockFlipped_y);
                    }
            }
        }

    return bitmap;
}
LOG_RETHROW

void Sm::readRomFromFile()
try
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
        throw std::runtime_error(LOG_INFO "Failed to open ROM");

    file.seekg(0, std::ios::end);
    const n_t fileSize = file.tellg();
    if (fileSize != n_rom)
        throw std::runtime_error(LOG_INFO "Incorrect ROM size");

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(std::data(rom)), std::size(rom));
    if (!file)
        throw std::runtime_error(LOG_INFO "Failed to read ROM");
}
LOG_RETHROW

void Sm::loadTilesetTable()
try
{
    DataReader reader(rom);
    reader.seek(0x8F'E6A2_sm);
    for (TilesetHeader& header : tilesetHeaders)
        header = TilesetHeader(reader);
}
LOG_RETHROW

void Sm::loadRoom(SnesAddress roomAddress)
try
{
    if (roomHeaders.contains(roomAddress))
        return;

    DataReader reader(rom);
    reader.seek(roomAddress);
    sm::RoomHeader roomHeader(reader);
    for (SnesAddress stateAddress : roomHeader.getStateAddresses())
        loadState(stateAddress);

    roomHeaders[roomAddress] = std::move(roomHeader);
}
LOG_RETHROW

void Sm::loadState(SnesAddress stateAddress)
try
{
    if (stateHeaders.contains(stateAddress))
        return;

    DataReader reader(rom);
    reader.seek(stateAddress);
    sm::StateHeader stateHeader(reader);
    // todo extract more stuff

    stateHeaders[stateAddress] = std::move(stateHeader);
}
LOG_RETHROW

struct sm::Mode1Tileset
{
    using metatile_t = std::array<word_t, 4>; // Tilemap entries for 4 tiles: up-left, up-right, down-left, down-right
    using tileGfx_t = std::array<byte_t, 0x20>; // 4bpp planar
    using palette_t = std::array<word_t, 0x10>; // 15-bit BGR (with unused MSb)

    std::array<metatile_t, 0x400> metatiles;
    std::array<tileGfx_t, 0x400> tileGfxs;
    std::array<palette_t, 8> palettes;
};

struct sm::Mode7Tileset // Possibly too large for stack allocation
{
    using metatile_t = std::array<word_t, 4>;
    using tileGfx_t = std::array<byte_t, 0x40>;
    using palette_t = std::array<word_t, 0x80>;

    std::array<metatile_t, 0x400> metatiles; // Always 2000h bytes
    std::array<tileGfx_t, 0x200> tileGfxs; // Always 8000h bytes
    palette_t palette; // Always 100h bytes
};

Mode1Tileset Sm::makeMode1Tileset(index_t i_tileset, bool isExtraLarge, bool isCeres) const
try
{
    const TilesetHeader& tilesetHeader = tilesetHeaders[i_tileset];
    DataReader compressedReader(rom);
    Mode1Tileset tileset;

    // Metatiles
    {
        const n_t n_compressed_expected = !isCeres ? 0x1800 : 0x2000;
        compressedReader.seek(SnesAddress(tilesetHeader.metatilesAddress));
        const std::vector<byte_t> decompressed = decompress(compressedReader, n_compressed_expected);
        const n_t n_decompressed = std::size(decompressed);
        if (n_decompressed != n_compressed_expected)
            throw std::runtime_error(LOG_INFO "Bad tileset metatiles size");

        std::span<Mode1Tileset::metatile_t> sceMetatiles(tileset.metatiles);
        if (!isCeres)
        {
            // Load CRE metatiles
            compressedReader.seek(0xB9'A09D_sm);
            const std::vector<byte_t> decompressed = decompress(compressedReader, 0x800);
            const n_t n_decompressed = std::size(decompressed);
            if (n_decompressed != 0x800)
                throw std::runtime_error(LOG_INFO "Bad CRE metatiles size");

            DataReader reader(decompressed);
            for (index_t i{}; i < n_decompressed / 8; ++i)
                tileset.metatiles[i] = reader.readInts<2, 4>();

            sceMetatiles = sceMetatiles.subspan(0x100);
        }

        DataReader reader(decompressed);
        for (Mode1Tileset::metatile_t& metatile : sceMetatiles)
            metatile = reader.readInts<2, 4>();
    }

    // Tile GFX
    {
        compressedReader.seek(SnesAddress(tilesetHeader.tileGfxsAddress));
        const std::vector<byte_t> decompressed = decompress(compressedReader, 0x8000);
        const n_t n_decompressed = std::size(decompressed);
        if (n_decompressed % 0x20 != 0)
            throw std::runtime_error(LOG_INFO "Bad tileset tile GFXs size");

        DataReader reader(decompressed);
        for (index_t i{}; i < n_decompressed / 0x20; ++i)
            tileset.tileGfxs[i] = reader.readInts<1, 0x20>();

        if (!isExtraLarge)
        {
            // Load CRE tile GFX
            compressedReader.seek(0xB9'8000_sm);
            const std::vector<byte_t> decompressed = decompress(compressedReader, 0x3000);
            const n_t n_decompressed = std::size(decompressed);
            if (n_decompressed != 0x3000)
                throw std::runtime_error(LOG_INFO "Bad CRE tile GFXs size");

            DataReader reader(decompressed);
            for (index_t i{}; i < n_decompressed / 0x20; ++i)
                tileset.tileGfxs[0x280 + i] = reader.readInts<1, 0x20>();
        }
    }

    // Palettes
    {
        compressedReader.seek(SnesAddress(tilesetHeader.palettesAddress));
        const std::vector<byte_t> decompressed = decompress(compressedReader, 0x100);
        const n_t n_decompressed = std::size(decompressed);
        if (n_decompressed != 0x100)
            throw std::runtime_error(LOG_INFO "Bad tileset palettes size");

        DataReader reader(decompressed);
        for (Mode1Tileset::palette_t& palette : tileset.palettes)
            palette = reader.readInts<2, 0x10>();
    }

    return tileset;
}
LOG_RETHROW

static metatileBitmaps_t drawMode1Metatile(Mode1Tileset::metatile_t metatile, const Mode1Tileset& tileset)
try
{
    metatileBitmaps_t bitmaps;
    for (index_t i_y_tile{}; i_y_tile < 2; ++i_y_tile)
        for (index_t i_x_tile{}; i_x_tile < 2; ++i_x_tile)
        {
            MetatileBitmap& bitmap = bitmaps[i_y_tile][i_x_tile];
            const word_t tilemapEntry = metatile[i_y_tile * 2 + i_x_tile];
            
            const index_t i_tileGfx = tilemapEntry & 0x3FF;
            const index_t i_palette = tilemapEntry >> 0xA & 7;
            const bool hasPriority = tilemapEntry >> 0xD & 1;
            const bool isFlipped_x = tilemapEntry >> 0xE & 1;
            const bool isFlipped_y = tilemapEntry >> 0xF;

            const Mode1Tileset::palette_t& palette = tileset.palettes[i_palette];
            if (i_tileGfx > std::size(tileset.tileGfxs))
                throw std::out_of_range(LOG_INFO "Out of bounds tile GFX index");

            bitmap.hasPriority = hasPriority;

            const Mode1Tileset::tileGfx_t& tileGfx = tileset.tileGfxs[i_tileGfx];
            for (index_t y{}; y < 8; ++y)
                for (index_t x{}; x < 8; ++x)
                {
                    const index_t y_pixel = !isFlipped_y ? y : 7 - y;
                    const index_t x_pixel = !isFlipped_x ? x : 7 - x;

                    // Bitplane decoding
                    index_t i_colour{};
                    for (index_t i{}; i < 2; ++i)
                        for (index_t ii{}; ii < 2; ++ii)
                            i_colour |= (tileGfx[i * 0x10 + y * 2 + ii] >> 7 - x & 1) << i * 2 + ii;

                    uint16_t colour = palette[i_colour] & 0x7FFF;
                    if (i_colour == 0)
                        colour |= 0x8000; // Transparent

                    bitmap.data[y_pixel][x_pixel].colour = colour;
                }
        }

    return bitmaps;
}
LOG_RETHROW

std::vector<metatileBitmaps_t> Sm::drawMode1Tileset(index_t i_tileset, bool isExtraLarge, bool isCeres) const
try
{
    const Mode1Tileset tileset = makeMode1Tileset(i_tileset, isExtraLarge, isCeres);
    const n_t n_metatiles = std::size(tileset.metatiles);
    std::vector<metatileBitmaps_t> bitmaps;
    bitmaps.reserve(n_metatiles);
    for (const Mode1Tileset::metatile_t metatile : tileset.metatiles)
        bitmaps.push_back(drawMode1Metatile(metatile, tileset));

    return bitmaps;
}
LOG_RETHROW
