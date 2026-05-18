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


FxHeaderEntry::FxHeaderEntry(DataReader& reader)
try
{
    address = reader.tell();
    reader.readIntsTo<2>(doorAddress, baseYPosition, targetYPosition, yVelocity);
    reader.readIntsTo<1>(timer, type, defaultLayerBlend, fxLayer3LayerBlend, liquidOptions, paletteFxBitset, animatedTilesBitset, paletteBlend);
}
LOG_RETHROW


FxHeader::FxHeader(DataReader& reader)
try
{
    address = reader.tell();
    while (reader.peekInt<2>() != 0xFFFF)
    {
        entries.push_back(FxHeaderEntry(reader));
        if (entries.back().doorAddress == 0)
            break;
    }
}
LOG_RETHROW


TilesetHeader::TilesetHeader(DataReader& reader)
try
{
    address = reader.tell();
    reader.readIntsTo<3>(metatilesAddress, tileGfxsAddress, palettesAddress);
}
LOG_RETHROW


AnimatedTilesHeader::AnimatedTilesHeader(DataReader& reader)
try
{
    address = reader.tell();
    reader.readIntsTo<2>(instructionListAddress, size, vramAddress);
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
    loadRoom(0x8F'D78F_sm); // Pre-Draygon room
    loadRoom(0x8F'AF14_sm); // Lava dive room
    loadRoom(0x8F'9D19_sm); // Charge beam room
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

struct sm::TileBitmap
{
    std::array<std::array<rom::Abgr16, 8>, 8> data;
    bool hasPriority;
};

static TileBitmap draw2bppTile(
    word_t tilemapEntry, 
    std::span<const std::array<byte_t, 0x10>> tileGfxs, // todo
    std::span<const Mode1Tileset::palette_t, 2> palettes // todo
)
try
{
    TileBitmap bitmap;
            
    const index_t i_tileGfx = tilemapEntry & 0x3FF;
    const index_t i_palette = tilemapEntry >> 0xA & 7;
    const bool hasPriority = tilemapEntry >> 0xD & 1;
    const bool isFlipped_x = tilemapEntry >> 0xE & 1;
    const bool isFlipped_y = tilemapEntry >> 0xF;

    if (i_tileGfx > std::size(tileGfxs))
        throw std::out_of_range(LOG_INFO "Out of bounds tile GFX index");

    bitmap.hasPriority = hasPriority;

    std::span<const word_t, 4> palette(&palettes[i_palette / 4][i_palette % 4 * 4], 4);
    const std::array<byte_t, 0x10>& tileGfx = tileGfxs[i_tileGfx];
    for (index_t y{}; y < 8; ++y)
        for (index_t x{}; x < 8; ++x)
        {
            const index_t y_pixel = !isFlipped_y ? y : 7 - y;
            const index_t x_pixel = !isFlipped_x ? x : 7 - x;

            // Bitplane decoding
            index_t i_colour{};
            for (index_t i{}; i < 2; ++i)
                i_colour |= (tileGfx[y * 2 + i] >> 7 - x & 1) << i;

            uint16_t colour = palette[i_colour] & 0x7FFF;
            if (i_colour == 0)
                colour |= 0x8000; // Transparent

            bitmap.data[y_pixel][x_pixel].colour = colour;
        }

    return bitmap;
}
LOG_RETHROW

static TileBitmap draw4bppTile(word_t tilemapEntry, std::span<const Mode1Tileset::tileGfx_t> tileGfxs, std::span<const Mode1Tileset::palette_t, 8> palettes)
try
{
    TileBitmap bitmap;
            
    const index_t i_tileGfx = tilemapEntry & 0x3FF;
    const index_t i_palette = tilemapEntry >> 0xA & 7;
    const bool hasPriority = tilemapEntry >> 0xD & 1;
    const bool isFlipped_x = tilemapEntry >> 0xE & 1;
    const bool isFlipped_y = tilemapEntry >> 0xF;

    bitmap.hasPriority = hasPriority;

    const Mode1Tileset::palette_t& palette = palettes[i_palette];
    const Mode1Tileset::tileGfx_t& tileGfx = tileGfxs[i_tileGfx];
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

    return bitmap;
}
LOG_RETHROW

static metatileBitmaps_t drawMode1Metatile(Mode1Tileset::metatile_t metatile, const Mode1Tileset& tileset)
try
{
    metatileBitmaps_t bitmaps;
    for (index_t i_y_tile{}; i_y_tile < 2; ++i_y_tile)
        for (index_t i_x_tile{}; i_x_tile < 2; ++i_x_tile)
        {
            const word_t tilemapEntry = metatile[i_y_tile * 2 + i_x_tile];
            bitmaps[i_y_tile][i_x_tile] = draw4bppTile(tilemapEntry, tileset.tileGfxs, tileset.palettes);
        }

    return bitmaps;
}
LOG_RETHROW

static std::vector<metatileBitmaps_t> drawMode1Tileset(const Mode1Tileset& tileset)
try
{
    const n_t n_metatiles = std::size(tileset.metatiles);
    std::vector<metatileBitmaps_t> bitmaps;
    bitmaps.reserve(n_metatiles);
    for (const Mode1Tileset::metatile_t metatile : tileset.metatiles)
        bitmaps.push_back(drawMode1Metatile(metatile, tileset));

    return bitmaps;
}
LOG_RETHROW


struct sm::ZPixel
{
    enum Priority
    {
        backdrop,
        bg3_0,
        sprites_0,
        sprites_1,
        bg2_0,
        bg1_0,
        sprites_2,
        bg2_1,
        bg1_1,
        sprites_3,
        bg3_1
    };

    rom::Abgr16 colour;
    Priority priority;
};

static void drawBlock(Array2d<ZPixel>& bitmap, index_t x_block, index_t y_block, word_t block, bool isBackground, const std::vector<metatileBitmaps_t>& tileset)
try
{
    const index_t i_metatile = block & 0x3FF;
    const bool isBlockFlipped_x = block >> 0xA & 1;
    const bool isBlockFlipped_y = block >> 0xB & 1;
            
    if (i_metatile > std::size(tileset))
        if (isBackground)
            throw std::out_of_range(LOG_INFO "Out of bounds background metatile index");
        else
            throw std::out_of_range(LOG_INFO "Out of bounds level metatile index");
            
    const metatileBitmaps_t& metatile = tileset[i_metatile];
    for (index_t i_y_tile{}; i_y_tile < 2; ++i_y_tile)
        for (index_t i_x_tile{}; i_x_tile < 2; ++i_x_tile)
        {  
            const index_t
                i_x_blockTile = !isBlockFlipped_x ? i_x_tile : 1 - i_x_tile,
                i_y_blockTile = !isBlockFlipped_y ? i_y_tile : 1 - i_y_tile;

            const TileBitmap& tile = metatile[i_y_blockTile][i_x_blockTile];
            ZPixel::Priority tilePriority;
            if (isBackground)
                tilePriority = tile.hasPriority ? ZPixel::bg2_1 : ZPixel::bg2_0;
            else
                tilePriority = tile.hasPriority ? ZPixel::bg1_1 : ZPixel::bg1_0;
            
            for (index_t y{}; y < 8; ++y)
                for (index_t x{}; x < 8; ++x)
                {
                    const index_t
                        y_dest = y_block * 0x10 + i_y_tile * 8 + y,
                        x_dest = x_block * 0x10 + i_x_tile * 8 + x,
                        y_src = !isBlockFlipped_y ? y : 7 - y,
                        x_src = !isBlockFlipped_x ? x : 7 - x;
                        
                    const rom::Abgr16 tileColour = tile.data[y_src][x_src];
                    if (tileColour.colour & 0x8000)
                        continue;

                    ZPixel& bitmapPixel = bitmap[y_dest][x_dest];
                    bitmapPixel.colour = tileColour;
                    bitmapPixel.priority = tilePriority;
                }
    }
}
LOG_RETHROW

static Array2d<ZPixel> drawRoomBlocks(
    n_t n_y_blocks, n_t n_x_blocks, const std::vector<metatileBitmaps_t>& tileset, std::span<const word_t> blocks, bool isBackground
)
try
{
    Array2d<ZPixel> bitmap({.n_y = n_y_blocks * 0x10, .n_x = n_x_blocks * 0x10});
    std::fill_n(std::data(bitmap), std::size(bitmap), ZPixel{0x8000});
    if (blocks.empty())
        return bitmap;

    for (index_t y_block{}; y_block < n_y_blocks; ++y_block)
        for (index_t x_block{}; x_block < n_x_blocks; ++x_block)
        {
            const word_t block = blocks[y_block * n_x_blocks + x_block];
            drawBlock(bitmap, x_block, y_block, block, isBackground, tileset);
        }

    return bitmap;
}
LOG_RETHROW

Array2d<ZPixel> Sm::drawRoomFx(
    const FxHeaderEntry* p_fx, n_t n_y_tiles, n_t n_x_tiles, std::span<const std::array<word_t, 0x10>, 8> palettes
) const
try
{
    using tileGfx_t = std::array<byte_t, 0x10>;

    Array2d<ZPixel> bitmap({.n_y = n_y_tiles * 8, .n_x = n_x_tiles * 8});
    std::fill_n(std::data(bitmap), std::size(bitmap), ZPixel{{palettes[1][0xB]}, ZPixel::bg3_0});

    if (!p_fx)
        return bitmap;

    const bool isLiquid = std::ranges::contains(std::array{2, 4, 6, 0x26}, p_fx->type);
    if (isLiquid && p_fx->baseYPosition & 0x8000)
        return bitmap;

    DataReader reader(rom);
    const word_t tilemapAddressWord = reader.peekInt<2>(SnesAddress(0x83'ABF0 + p_fx->type));
    if (!tilemapAddressWord)
        return bitmap;

    reader.seek(0x9A'B200_sm); // Standard BG3 tiles
    std::array<tileGfx_t, 0x100> tileGfxs;
    for (tileGfx_t& tileGfx : tileGfxs)
        tileGfx = reader.readInts<1, 0x10>();

    SnesAddress animatedTilesAddress{};
    switch (p_fx->type)
    {
    case 2: // Lava
        animatedTilesAddress = 0x87'82AB_sm;
        break;

    case 4: // Acid
        animatedTilesAddress = 0x87'82C9_sm;
        break;

    case 8: // Spores
        animatedTilesAddress = 0x87'82FD_sm;
        break;

    case 0xA: // Rain
        animatedTilesAddress = 0x87'82E7_sm;
        break;
    }

    if (animatedTilesAddress)
    {
        reader.seek(animatedTilesAddress);
        AnimatedTilesHeader animatedTiles(reader);
        if (animatedTiles.vramAddress >= 0x4800)
            throw std::runtime_error(LOG_INFO + std::format("Invalid animated tiles VRAM address ${:X}", animatedTiles.vramAddress));

        if (animatedTiles.vramAddress >= 0x4000)
        {
            reader.seek(SnesAddress(0x87'0000 | animatedTiles.instructionListAddress));
            if (reader.readInt<2>() & 0x8000)
                throw std::runtime_error(LOG_INFO "Unsupported animated tiles instruction list");

            const word_t gfxAddress = reader.readInt<2>();
            if (gfxAddress < 0x8000)
                throw std::runtime_error(LOG_INFO "Invalid animated tiles special instruction");

            reader.seek(SnesAddress(0x87'0000 | gfxAddress));
            const index_t i_tileGfx = (animatedTiles.vramAddress - 0x4000) / 8;
            for (tileGfx_t& tileGfx : std::span(&tileGfxs[i_tileGfx], animatedTiles.size))
                tileGfx = reader.readInts<1, 0x10>();
        }
    }


    const index_t baseYPosition = isLiquid ? p_fx->baseYPosition : 0;
    const SnesAddress tilemapAddress(0x8A'0000 | tilemapAddressWord);
    reader.seek(tilemapAddress);

    for (index_t y_tile{}; y_tile < 0x21; ++y_tile)
        for (index_t x_tile{}; x_tile < 0x20; ++x_tile)
        {
            index_t y_dest = baseYPosition + y_tile * 8;
            if (y_dest >= bitmap.getSizes().n_y)
                return bitmap;

            const word_t tilemapEntry = reader.readInt<2>();
            const TileBitmap tileBitmap = draw2bppTile(tilemapEntry, tileGfxs, palettes.subspan<0, 2>());
            const ZPixel::Priority tilePriority = tileBitmap.hasPriority ? ZPixel::bg3_1 : ZPixel::bg3_0;

            const n_t n_y = std::min<n_t>(8, bitmap.getSizes().n_y - y_dest);
            for (index_t y_src{}; y_src < n_y; ++y_src, ++y_dest)
                for (index_t x_repeat{}; x_repeat * 0x20 + x_tile < n_x_tiles; ++x_repeat)
                    for (index_t x_src{}; x_src < 8; ++x_src)
                    {
                        const rom::Abgr16 tileColour = tileBitmap.data[y_src][x_src];
                        if (tileColour.colour & 0x8000)
                            continue;

                        const index_t x_dest = (x_repeat * 0x20 + x_tile) * 8 + x_src;
                        ZPixel& bitmapPixel = bitmap[y_dest][x_dest];
                        bitmapPixel.colour = tileColour;
                        bitmapPixel.priority = tilePriority;
                    }
        }

    // Ran out of FX tilemap to draw...
    // Lets just repeat the last 4 rows
    const index_t y_padding = baseYPosition + 0x21 * 8;
    for (index_t y_bitmap = y_padding; y_bitmap < bitmap.getSizes().n_y; ++y_bitmap)
        std::ranges::copy(bitmap[y_padding - 0x20 + (y_bitmap - y_padding) % 0x20], std::begin(bitmap[y_bitmap]));

    return bitmap;

}
LOG_RETHROW

static void addBitmap(Array2d<ZPixel>& destBitmap, const Array2d<ZPixel>& srcBitmap)
{
    const array2d::Sizes sizes = destBitmap.getSizes();
    for (index_t y{}; y < sizes.n_y; ++y)
        for (index_t x{}; x < sizes.n_x; ++x)
        {
            const ZPixel srcPixel = srcBitmap[y][x];
            const rom::Abgr16 srcColour = srcPixel.colour;
            if (srcColour.colour & 0x8000)
                continue;

            ZPixel& destPixel = destBitmap[y][x];
            rom::Abgr16& destColour = destPixel.colour;
            if (destColour.colour & 0x8000 || srcPixel.priority > destPixel.priority)
                destPixel = srcPixel;
        }
}

static void colourMathAdd(
    Array2d<ZPixel>& mainScreen, const Array2d<ZPixel>& subScreen, 
    word_t mainScreenBackdrop, word_t subScreenBackdrop = 0
)
{
    const array2d::Sizes sizes = mainScreen.getSizes();
    for (index_t y{}; y < sizes.n_y; ++y)
        for (index_t x{}; x < sizes.n_x; ++x)
        {
            rom::Abgr16& mainColour = mainScreen[y][x].colour;
            const rom::Abgr16 subColour = subScreen[y][x].colour;
            word_t mainColourValue = mainColour.colour;
            if (mainColourValue & 0x8000)
                mainColourValue = mainScreenBackdrop;
            
            word_t subColourValue = subColour.colour;
            if (subColourValue & 0x8000)
                subColourValue = subScreenBackdrop;

            const unsigned
                red   = std::min<unsigned>(0x1F, (mainColourValue      & 0x1F) + (subColourValue        & 0x1F)),
                green = std::min<unsigned>(0x1F, (mainColourValue >> 5 & 0x1F) + (subColourValue >> 5   & 0x1F)),
                blue  = std::min<unsigned>(0x1F, (mainColourValue >> 0xA)      + (subColourValue >> 0xA));

            mainColour.colour = red | green << 5 | blue << 0xA;
        }
}

static void colourMathSubtract(
    Array2d<ZPixel>& mainScreen, const Array2d<ZPixel>& subScreen, 
    word_t mainScreenBackdrop, word_t subScreenBackdrop = 0
)
{
    const array2d::Sizes sizes = mainScreen.getSizes();
    for (index_t y{}; y < sizes.n_y; ++y)
        for (index_t x{}; x < sizes.n_x; ++x)
        {
            rom::Abgr16& mainColour = mainScreen[y][x].colour;
            const rom::Abgr16 subColour = subScreen[y][x].colour;
            word_t mainColourValue = mainColour.colour;
            if (mainColourValue & 0x8000)
                mainColourValue = mainScreenBackdrop;
            
            word_t subColourValue = subColour.colour;
            if (subColourValue & 0x8000)
                subColourValue = subScreenBackdrop;

            const unsigned
                red   = std::max(0, signed(mainColourValue      & 0x1F) - signed(subColourValue        & 0x1F)),
                green = std::max(0, signed(mainColourValue >> 5 & 0x1F) - signed(subColourValue >> 5   & 0x1F)),
                blue  = std::max(0, signed(mainColourValue >> 0xA)      - signed(subColourValue >> 0xA));

            mainColour.colour = red | green << 5 | blue << 0xA;
        }
}

static Array2d<rom::Abgr16> stripPriority(const Array2d<ZPixel>& bitmap)
try
{
    const array2d::Sizes sizes = bitmap.getSizes();
    Array2d<rom::Abgr16> stripped(sizes);
    for (index_t y{}; y < sizes.n_y; ++y)
        for (index_t x{}; x < sizes.n_x; ++x)
            stripped[y][x] = bitmap[y][x].colour;

    return stripped;
}
LOG_RETHROW

static Array2d<rom::Abgr16> blendLayers(
    Array2d<ZPixel> bg1Bitmap, Array2d<ZPixel> bg2Bitmap, Array2d<ZPixel> bg3Bitmap, 
    const FxHeaderEntry* p_fxEntry, word_t mainScreenBackdrop
)
try
{
    byte_t layerBlend = 0;
    if (p_fxEntry)
    {
        if (std::ranges::contains(std::array{2, 4, 6, 8, 0xA, 0xC, 0x26}, p_fxEntry->type))
            layerBlend = p_fxEntry->fxLayer3LayerBlend;
        else
            layerBlend = p_fxEntry->defaultLayerBlend;
    }
    
    switch (layerBlend)
    {
    default:
        throw std::runtime_error(LOG_INFO + std::format("Unknown layer blend {:X}h", layerBlend));

    // Normal
    case 0:
    case 2:
    case 0xE:
    case 0x20:
    {
        // BG3 blended onto BG1/BG2/sprites

        addBitmap(bg1Bitmap, bg2Bitmap);
        // add sprites to bg1Bitmap
        colourMathAdd(bg1Bitmap, bg3Bitmap, mainScreenBackdrop);
        return stripPriority(bg1Bitmap);
    }
        

    // Coven (low priority sprites are semi-transparent on BG2)
    case 8:
    {
        // BG3/sprites blended onto BG2, BG1/sprites

        // add sprites to bg3Bitmap
        colourMathAdd(bg2Bitmap, bg3Bitmap, mainScreenBackdrop);
        addBitmap(bg2Bitmap, bg1Bitmap);
        // add sprites to bg2Bitmap
        return stripPriority(bg2Bitmap);
    }

    // Spores (BG3 hidden by BG1)
    case 0xA:
    {
        // BG3 blended onto BG2/sprites, BG1

        // add sprites to bg2Bitmap
        colourMathAdd(bg2Bitmap, bg3Bitmap, mainScreenBackdrop);
        addBitmap(bg2Bitmap, bg1Bitmap);
        return stripPriority(bg2Bitmap);
    }

    // Water - dimmed by BG3
    case 0x14:
    case 0x22:
    {
        // BG3 inverse blended onto BG1/BG2/sprites

        addBitmap(bg1Bitmap, bg2Bitmap);
        // add sprites to bg1Bitmap
        colourMathSubtract(bg1Bitmap, bg3Bitmap, mainScreenBackdrop);
        return stripPriority(bg1Bitmap);
    }

    // Water - background waterfalls (dimmed by BG2/BG3)
    case 0x16:
    {
        // BG2/BG3 inverse blended onto BG1/sprites
        
        // add sprites to bg1Bitmap
        addBitmap(bg2Bitmap, bg3Bitmap);
        colourMathSubtract(bg1Bitmap, bg2Bitmap, mainScreenBackdrop);
        return stripPriority(bg1Bitmap);
    }

    // Colour math affects all sprite palettes (semi-transparent BG1/BG2/sprites on BG3)
    case 0x18:
    case 0x1E:
    case 0x30:
    {
        // BG1/BG2/sprites blended onto BG3

        addBitmap(bg1Bitmap, bg2Bitmap);
        // add sprites to bg1Bitmap
        colourMathAdd(bg3Bitmap, bg1Bitmap, mainScreenBackdrop);
        return stripPriority(bg3Bitmap);
    }

    // Red desaturation (BG3 disabled, dimmed by red subscreen backdrop)
    case 0x28:
    {
        // Red colour inverse blended onto BG1/BG2/sprites

        addBitmap(bg1Bitmap, bg2Bitmap);
        // add sprites to bg1Bitmap
        return stripPriority(bg1Bitmap);
    }

    // Orange desaturation (BG3 disabled, dimmed by orange subscreen backdrop)
    case 0x2A:
    {
        // Orange colour inverse blended onto BG1/BG2/sprites

        addBitmap(bg1Bitmap, bg2Bitmap);
        // add sprites to bg1Bitmap
        return stripPriority(bg1Bitmap);
    }
    }
}
LOG_RETHROW

Array2d<rom::Abgr16> Sm::drawRoom(rom::Address address) const
try
{
    const RoomHeader& roomHeader = roomHeaders.at(address);
    const StateHeader& stateHeader = stateHeaders.at(roomHeader.getStateAddresses().back());

    DataReader reader(rom);
    reader.seek(SnesAddress(stateHeader.levelDataAddress));
    const LevelData levelData(reader);

    reader.seek(SnesAddress(0x83'0000 | stateHeader.fxAddress));
    const FxHeader fxHeader(reader);
    const FxHeaderEntry* p_fxEntry{};
    if (!fxHeader.entries.empty())
        p_fxEntry = &fxHeader.entries.back();

    const bool isExtraLarge = roomHeader.creBitset & 4;
    const bool isCeres = roomHeader.areaIndex == 6;
    Mode1Tileset tileset = makeMode1Tileset(stateHeader.tilesetIndex, isExtraLarge, isCeres);
    if (p_fxEntry && p_fxEntry->paletteBlend)
        std::ranges::copy(reader.peekInts<2, 3>(SnesAddress(0x89'AA02 + p_fxEntry->paletteBlend)), &tileset.palettes[1][9]);

    const std::vector<metatileBitmaps_t> metatileBitmaps = drawMode1Tileset(tileset);
    // todo: load BG1/2 animated tiles

    const n_t
        n_y_blocks = roomHeader.height * 0x10, 
        n_x_blocks = roomHeader.width * 0x10;

    std::span<const word_t> level(levelData.level), background(levelData.background);
    Array2d<ZPixel>
        bg1Bitmap = drawRoomBlocks(n_y_blocks, n_x_blocks, metatileBitmaps, level, false),
        bg2Bitmap = drawRoomBlocks(n_y_blocks, n_x_blocks, metatileBitmaps, background, true),
        bg3Bitmap = drawRoomFx(p_fxEntry, n_y_blocks * 2, n_x_blocks * 2, tileset.palettes);
    
    const word_t mainScreenBackdrop = tileset.palettes[0][0];

    return blendLayers(std::move(bg1Bitmap), std::move(bg2Bitmap), std::move(bg3Bitmap), p_fxEntry, mainScreenBackdrop);
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
