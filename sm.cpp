#include "sm.h"

#include "util/graphics.h"
#include "global.h"

#include <array>
#include <numeric>
#include <string_view>


// Public member functions
Sm::Sm(std::filesystem::path filepath)
try
    : Rom(filepath)
{
    Reader r(makeReader());
    if (std::array title(r.get<0x15, char>(0x80FFC0_sm)); std::string_view(std::data(title), std::size(title)) != "Super Metroid        "s)
        throw std::runtime_error("Invalid Super Metroid ROM (incorrect header title)"s);

    if (byte_t region(r.get<byte_t>(0x80FFD9_sm)); region != 0)
        if (region == 2)
            throw std::runtime_error("PAL Super Metroid not supported"s);
        else
            throw std::runtime_error("Invalid Super Metroid ROM (incorrect header region)"s);

    findRoomHeaders();
}
LOG_RETHROW

void Sm::drawLevelView(Cairo::RefPtr<Cairo::Surface> p_surface, unsigned x, unsigned y) const
try
{
    if (!p_level)
        return;

    Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_surface));
    p_context->set_source(p_level, -signed(x * 16), -signed(y * 16));
    p_context->paint();
}
LOG_RETHROW

void Sm::drawSpritemapView(Cairo::RefPtr<Cairo::Surface> p_surface, unsigned scroll_x, unsigned scroll_y) const
try
{
    if (!p_spritemapSurface)
        return;

    Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_surface));
    p_context->scale(2, 2);
    p_context->set_source(p_spritemapSurface, -signed(scroll_x), -signed(scroll_y));
    p_context->paint();
}
LOG_RETHROW

void Sm::drawSpritemapTilesView(Cairo::RefPtr<Cairo::Surface> p_surface, unsigned scroll_x, unsigned scroll_y) const
try
{
    if (!p_spritemapTilesSurface)
        return;

    Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_surface));
    p_context->scale(2, 2);
    p_context->set_source(p_spritemapTilesSurface, -signed(scroll_x), -signed(scroll_y));
    p_context->paint();
}
LOG_RETHROW

auto Sm::getLevelViewDimensions() const -> Dimensions
try
{
    return {16, levelData.n_y * 0x10, levelData.n_x * 0x10};
}
LOG_RETHROW

auto Sm::getRoomList() const -> std::vector<RoomList>
try
{
    std::vector<RoomList> areaLists
    {
        {0, "0 - Crateria"s, {}},
        {1, "1 - Brinstar"s, {}},
        {2, "2 - Norfair"s, {}},
        {3, "3 - Wrecked Ship"s, {}},
        {4, "4 - Maridia"s, {}},
        {5, "5 - Tourian"s, {}},
        {6, "6 - Ceres"s, {}},
        {7, "7 - Debug"s, {}}
    };

    for (index_t i_roomHeader{}; i_roomHeader < std::size(knownRoomHeaders); ++i_roomHeader)
    {
        const auto& [p_roomHeader, roomHeader] = knownRoomHeaders[i_roomHeader];
        if (roomHeader.areaIndex >= 8)
            continue;

        RoomList roomList{long(i_roomHeader), "$"s + toHexString(word_t(p_roomHeader.snes())), {}};
        if (!std::empty(roomHeader.events))
        {
            roomList.subrooms.push_back({-1, "Default state"s, {}});
            for (index_t i_eventHeader{}; i_eventHeader < std::size(roomHeader.events); ++i_eventHeader)
                roomList.subrooms.push_back({long(i_eventHeader), "$"s + toHexString(roomHeader.events[i_eventHeader]->p_state), {}});
        }

        areaLists[roomHeader.areaIndex].subrooms.push_back(std::move(roomList));
    }

    return areaLists;
}
LOG_RETHROW

void Sm::loadLevelData(std::vector<long> ids)
try
{
    Reader r(makeReader());
    RoomHeader::StateHeader stateHeader;
    const auto& [p_roomHeader, roomHeader](knownRoomHeaders[ids[1]]);
    if (std::size(ids) == 2 || ids[2] == -1)
        stateHeader = roomHeader.defaultState;
    else
    {
        r.seek(0x8F'8000_sm | roomHeader.events[ids[2]]->p_state);
        stateHeader = RoomHeader::StateHeader(r);
    }

    DebugFile(DebugFile::info) << LOG_INFO "Loading level data, address = "s << toHexString(stateHeader.p_levelData) << '\n';

    r.seek(Pointer(stateHeader.p_levelData));
    levelData = LevelData(roomHeader.height, roomHeader.width, !((stateHeader.layer2ScrollX | stateHeader.layer2ScrollY) & 1), r);

    // Now to create the Cairo layer1 and possibly layer2 image surfaces
    loadTileset(stateHeader.i_tileset);
    p_level = Util::makeImageSurface(int(levelData.n_x * 0x100), int(levelData.n_y * 0x100));
    if (levelData.layer2)
    {
        p_layer2 = createLayerSurface(levelData.layer2);
        Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_level));
        p_context->set_source(p_layer2, 0, 0);
        p_context->paint();
    }

    p_layer1 = createLayerSurface(levelData.layer1);
    Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_level));
    p_context->set_source(p_layer1, 0, 0);
    p_context->paint();
}
LOG_RETHROW

void Sm::loadSpritemap(index_t tilesAddress, index_t palettesAddress, index_t spritemapAddress, index_t tilesDestAddress, index_t palettesDestAddress)
try
{
    Pointer p_tilesAddress, p_palettesAddress, p_spritemapAddress, p_tilesDestAddress, p_palettesDestAddress;
    
    try
    {
        p_tilesAddress = Pointer(long_t(tilesAddress));
    }
    catch (const std::exception& e)
    {
        LOG_IGNORE(e);
        throw std::runtime_error(LOG_INFO "Invalid SNES address for tiles address");
    }

    try
    {
        p_palettesAddress = Pointer(long_t(palettesAddress));
    }
    catch (const std::exception& e)
    {
        LOG_IGNORE(e);
        throw std::runtime_error(LOG_INFO "Invalid SNES address for palettes address");
    }

    try
    {
        p_spritemapAddress = Pointer(long_t(spritemapAddress));
    }
    catch (const std::exception& e)
    {
        LOG_IGNORE(e);
        throw std::runtime_error(LOG_INFO "Invalid SNES address for spritemap address");
    }

    if (tilesDestAddress < 0x6000 || tilesDestAddress >= 0x8000 || tilesDestAddress % 0x10 != 0)
        throw std::runtime_error(LOG_INFO "Invalid VRAM address for tiles dest address");

    if (palettesDestAddress < 0x80 || palettesDestAddress >= 0x100)
        throw std::runtime_error(LOG_INFO "Invalid CGRAM address for palettes dest address");

    createSpritemapSurface(p_tilesAddress, p_palettesAddress, p_spritemapAddress, tilesDestAddress, palettesDestAddress);
    createSpritemapTilesSurface(p_tilesAddress, p_palettesAddress, tilesDestAddress, palettesDestAddress);
}
LOG_RETHROW


// Nested class member functions
Sm::Reader Sm::makeReader(Pointer address /* = {} */) const
try
{
    return Sm::Reader(filepath, address);
}
LOG_RETHROW

Sm::RoomHeader::RoomHeader(Reader& r)
{
    // Not logging exceptions

    roomIndex = r.get<byte_t>();
    areaIndex = r.get<byte_t>();
    mapX = r.get<byte_t>();
    mapY = r.get<byte_t>();
    width = r.get<byte_t>();
    height = r.get<byte_t>();
    upScroller = r.get<byte_t>();
    downScroller = r.get<byte_t>();
    creBitset = r.get<byte_t>();
    p_doorList = r.get<word_t>();
    if (p_doorList < 0x8000)
        throw std::out_of_range("Invalid door list pointer: "s + std::to_string(p_doorList));

    while (r.peek<word_t>() != 0xE5E6)
        events.push_back(EventHeader::loadEventHeader(r));

    r.get<word_t>(); // Read off 0xE5E6
    defaultState = StateHeader(r);
}

auto Sm::RoomHeader::EventHeader::loadEventHeader(Reader& r) -> std::unique_ptr<Base>
{
    // Not logging exceptions

    word_t p_id(r.peek<word_t>());
    switch (p_id)
    {
    default:
        throw std::runtime_error("Invalid event header ID: "s + std::to_string(p_id));
        break;

    case 0xE5EB:
        return std::make_unique<Door>(r);

    case 0xE5FF:
        return std::make_unique<MainAreaBoss>(r);

    case 0xE612:
        return std::make_unique<Event>(r);

    case 0xE629:
        return std::make_unique<Boss>(r);

    case 0xE640:
        return std::make_unique<Morph>(r);

    case 0xE652:
        return std::make_unique<MorphMissiles>(r);

    case 0xE669:
        return std::make_unique<PowerBombs>(r);

    case 0xE678:
        return std::make_unique<SpeedBooster>(r);
    }
}

Sm::RoomHeader::StateHeader::StateHeader(Reader& r)
{
    // Not logging exceptions

    p_levelData = r.getLong();
    i_tileset = r.get<byte_t>();
    i_musicData = r.get<byte_t>();
    i_musicTrack = r.get<byte_t>();
    p_fx = r.get<word_t>();
    p_enemyPopulation = r.get<word_t>();
    p_enemySet = r.get<word_t>();
    layer2ScrollX = r.get<byte_t>();
    layer2ScrollY = r.get<byte_t>();
    p_scroll = r.get<word_t>();
    p_xray = r.get<word_t>();
    p_mainAsm = r.get<word_t>();
    p_plm = r.get<word_t>();
    p_bg = r.get<word_t>();
    p_setupAsm = r.get<word_t>();

    if (p_levelData < 0x80'8000)
        throw std::out_of_range("Invalid level data pointer"s);

    if (p_fx && p_fx < 0x8000)
        throw std::out_of_range("Invalid FX pointer"s);

    if (p_enemyPopulation && p_enemyPopulation < 0x8000)
        throw std::out_of_range("Invalid enemy population pointer"s);

    if (p_enemySet && p_enemySet < 0x8000)
        throw std::out_of_range("Invalid enemy set pointer"s);

    if (p_xray && p_xray < 0x8000)
        throw std::out_of_range("Invalid special x-ray blocks pointer"s);

    if (p_mainAsm && p_mainAsm < 0x8000)
        throw std::out_of_range("Invalid main ASM pointer"s);

    if (p_plm && p_plm < 0x8000)
        throw std::out_of_range("Invalid PLM population pointer"s);

    if (p_bg && p_bg < 0x8000)
        throw std::out_of_range("Invalid library background pointer"s);

    if (p_setupAsm && p_setupAsm < 0x8000)
        throw std::out_of_range("Invalid setup ASM pointer"s);
}

Sm::LevelData::LevelData(n_t n_y, n_t n_x, bool isCustomLayer2, Reader& r)
try
    : n_y(n_y), n_x(n_x)
{
    {
        if (n_x == 0 || n_y == 0)
            throw std::runtime_error(LOG_INFO "Room size "s + std::to_string(n_x) + "x"s + std::to_string(n_y) + " has a zero dimension"s);

        if (n_x * n_y > 50)
            throw std::runtime_error(LOG_INFO "Room size "s + std::to_string(n_x) + "x"s + std::to_string(n_y) + " exceeds maximum of 50"s);

        const n_t maxScrollSize(0x10 * 0x10 * (2 + 1 + 2));
        n_t scrollSize(0x10 * 0x10 * (2 + 1));
        if (isCustomLayer2)
            scrollSize += 0x10 * 0x10 * 2;

        // Not using the room size to determine the decompress max size parameter as decompressed level data is permitted to exceed the room size
        // Using maximum room size instead
        std::vector<byte_t> decompressedData(decompress(50 * maxScrollSize + 2, r));
        if (std::size(decompressedData) < n_x * n_y * scrollSize + 2)
            throw std::runtime_error(LOG_INFO "Decompressed level data is too small for room. Expected "s + toHexString(n_x * n_y * scrollSize + 2) + " bytes, found "s + toHexString(std::size(decompressedData)) + " bytes"s);

        const word_t levelDataSize(decompressedData[0] | decompressedData[1] << 8);
        const n_t n_blocks(levelDataSize / 2);

        if (std::size(decompressedData) != n_blocks * (2 + 1) + 2 && std::size(decompressedData) != n_blocks * (2 + 1 + 2) + 2)
            DebugFile(DebugFile::warning) << LOG_INFO "Reported size of decompressed level data (" << toHexString(n_blocks) << " blocks) isn't consistent with the actual size of decompressed level data (" << toHexString(std::size(decompressedData)) << " bytes)";

        layer1 = Matrix<word_t>(n_y * 0x10, n_x * 0x10);
        std::copy_n(ByteCastIterator<word_t>(std::data(decompressedData) + 2), n_blocks, std::begin(layer1));

        bts = Matrix<byte_t>(n_y * 0x10, n_x * 0x10);
        std::copy_n(std::begin(decompressedData) + 2 + levelDataSize, n_blocks, std::begin(bts));

        if (isCustomLayer2)
        {
            layer2 = Matrix<word_t>(n_y * 0x10, n_x * 0x10);
            std::copy_n(ByteCastIterator<word_t>(std::data(decompressedData) + 2 + n_blocks * 3), n_blocks, std::begin(layer2));
        }
    }
}
LOG_RETHROW


// Private member functions
Cairo::RefPtr<Cairo::ImageSurface> Sm::createLayerSurface(const Matrix<word_t>& layer) const
{
    Cairo::RefPtr<Cairo::ImageSurface> p_layer(Util::makeImageSurface(int(layer.size_x() * 0x10), int(layer.size_y() * 0x10)));
    Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_layer));
    for (index_t y{}; y < layer.size_y(); ++y)
        for (index_t x{}; x < layer.size_x(); ++x)
        {
            const word_t i_block(layer[y][x] & 0x3FF);
            const bool
                flip_x(layer[y][x] >> 10 & 1),
                flip_y(layer[y][x] >> 11 & 1);

            p_context->set_source(Util::flip(metatileSurfaces[i_block], flip_x, flip_y), x * 16.0, y * 16.0);
            p_context->paint();
        }

    return p_layer;
}

void Sm::decompressTileset(index_t i_tileset)
try
{
    constexpr Pointer
        p_tilesetTable(0x8F'E7A7_sm),
        p_cre_tiles(0xB9'8000_sm),
        p_cre_metatiles(0xB9'A09D_sm);

    Reader r(makeReader());
    const Pointer
        p_tileset(0x8F'8000_sm | r.get<word_t>(p_tilesetTable + i_tileset * 2u)),
        p_metatiles{r.getLong(p_tileset)},
        p_tiles{r.getLong(p_tileset + 3u)},
        p_palettes{r.getLong(p_tileset + 6u)};

    r.seek(p_cre_metatiles);
    decompress(metatiles, r);

    r.seek(p_metatiles);
    decompress(metatiles + 0x100, 0x300, r);

    r.seek(p_tiles);
    decompress(tiles, r);

    r.seek(p_cre_tiles);
    decompress(tiles + 0x280, 0x180, r);

    r.seek(p_palettes);
    decompress(bgPalettes, r);
}
LOG_RETHROW

Cairo::RefPtr<Cairo::ImageSurface> Sm::createTileSurface(const tile_t& tile, const palette_t& palette, bool flip_x /*= false */, bool flip_y /*= false */) const
try
{
    Cairo::RefPtr<Cairo::ImageSurface> p_tileSurface(Util::makeImageSurface(8, 8));
    unsigned char* const tileSurfaceBuffer(p_tileSurface->get_data());

    for (index_t y{}; y < 8; ++y)
        for (index_t x{}; x < 8; ++x)
        {
            // Bitplane decoding
            index_t i_palette{};
            for (index_t i{}; i < 2; ++i)
                for (index_t ii{}; ii < 2; ++ii)
                    i_palette |= (tile[y * 2 + i * 0x10 + ii] >> 7 - x & 1) << i * 2 + ii;

            Util::bgr15ToRgba32(tileSurfaceBuffer + (y * 8 + x) * 4, palette[i_palette], i_palette == 0);
        }

    p_tileSurface->mark_dirty();
    return Util::flip(p_tileSurface, flip_x, flip_y);
}
LOG_RETHROW

Cairo::RefPtr<Cairo::ImageSurface> Sm::createMetatileSurface(const metatile_t& metatile) const
try
{
    Cairo::RefPtr<Cairo::ImageSurface> p_metatileSurface(Util::makeImageSurface(16, 16));

    for (index_t y{}; y < 2; ++y)
        for (index_t x{}; x < 2; ++x)
        {
            const word_t metatilePart(metatile[y * 2 + x]);
            const index_t
                i_tiles(metatilePart & 0x3FF),
                i_palettes(metatilePart >> 10 & 7);

            const bool
                bgPriority(metatilePart >> 13 & 1),
                flip_x(metatilePart >> 14 & 1),
                flip_y(metatilePart >> 15);

            Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_metatileSurface));
            Cairo::RefPtr<Cairo::ImageSurface> p_tileSurface(createTileSurface(tiles[i_tiles], bgPalettes[i_palettes], flip_x, flip_y));
            p_context->set_source(p_tileSurface, x * 8.0, y * 8.0);
            p_context->paint();
        }

    return p_metatileSurface;
}
LOG_RETHROW

void Sm::createMetatileSurfaces()
try
{
    for (index_t i{}; i < std::size(metatiles); ++i)
        metatileSurfaces[i] = createMetatileSurface(metatiles[i]);
}
LOG_RETHROW

void Sm::loadTileset(index_t i_tileset)
try
{
    decompressTileset(i_tileset);
    createMetatileSurfaces();
}
LOG_RETHROW

void Sm::createSpritemapSurface(Pointer p_tiles, Pointer p_palette, Pointer p_spritemapData, index_t tilesDestAddress, index_t palettesDestAddress)
try
{
    constexpr Pointer
        p_commonSpritePalettes(0x9A'8100_sm),
        p_commonSpriteTiles(0x9A'D200_sm);

    Reader r(makeReader());

    tile_t tiles[0x200]{};
    r.get<char, 1>(p_commonSpriteTiles, reinterpret_cast<char*>(tiles), 0x2A00);
    r.get<char, 1>(p_tiles, reinterpret_cast<char*>(tiles + (tilesDestAddress - 0x6000) / 0x10), sizeof(tile_t) * (0x8000 - tilesDestAddress) / 0x10);

    palette_t palettes[8]{};
    r.get<char, 1>(p_commonSpritePalettes, reinterpret_cast<char*>(palettes), sizeof(palettes));
    r.get<char, 1>(p_palette, reinterpret_cast<char*>(palettes) + (palettesDestAddress - 0x80) * 2, (0x100 - palettesDestAddress) * 2);

    r.seek(p_spritemapData);
    Spritemap spritemap(r);

    const n_t width(256), height(256), margin(128);
    p_spritemapSurface = Util::makeImageSurface(width, height);
    {
        Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_spritemapSurface));

        for (index_t priority{}; priority < 4; ++priority)
            for (const Spritemap::Entry& entry : spritemap.entries)
                if (entry.priority == priority)
                    if (!entry.large)
                    {
                        Cairo::RefPtr<Cairo::ImageSurface> p_tileSurface(createTileSurface(tiles[entry.i_tile], palettes[entry.i_palette], entry.flip_x, entry.flip_y));
                        p_context->set_source(p_tileSurface, double(entry.offset_x + margin), double(entry.offset_y + margin));
                        p_context->paint();
                    }
                    else
                    {
                        Cairo::RefPtr<Cairo::ImageSurface> p_metatileSurface(Util::makeImageSurface(16, 16));
                        for (index_t y{}; y <= n_t(entry.large); ++y)
                            for (index_t x{}; x <= n_t(entry.large); ++x)
                            {
                                Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_metatileSurface));
                                Cairo::RefPtr<Cairo::ImageSurface> p_tileSurface(createTileSurface(tiles[entry.i_tile + y * 0x10 + x], palettes[entry.i_palette]));
                                p_context->set_source(p_tileSurface, x * 8.0, y * 8.0);
                                p_context->paint();
                            }

                        p_context->set_source(Util::flip(p_metatileSurface, entry.flip_x, entry.flip_y), double(entry.offset_x + margin), double(entry.offset_y + margin));
                        p_context->paint();
                    }
    }
}
LOG_RETHROW

void Sm::createSpritemapTilesSurface(Pointer p_tiles, Pointer p_palette, index_t tilesDestAddress, index_t palettesDestAddress)
try
{
    constexpr Pointer
        p_commonSpritePalettes(0x9A'8100_sm),
        p_commonSpriteTiles(0x9A'D200_sm);

    Reader r(makeReader());

    tile_t tiles[0x200]{};
    r.get<char, 1>(p_commonSpriteTiles, reinterpret_cast<char*>(tiles), 0x2A00);
    r.get<char, 1>(p_tiles, reinterpret_cast<char*>(tiles + (tilesDestAddress - 0x6000) / 0x10), sizeof(tile_t) * (0x8000 - tilesDestAddress) / 0x10);

    palette_t palettes[8]{};
    r.get<char, 1>(p_commonSpritePalettes, reinterpret_cast<char*>(palettes), sizeof(palettes));
    r.get<char, 1>(p_palette, reinterpret_cast<char*>(palettes) + (palettesDestAddress - 0x80) * 2, (0x100 - palettesDestAddress) * 2);

    const n_t width(0x80), height(std::size(tiles) / 0x10 * 8);
    p_spritemapTilesSurface = Util::makeImageSurface(width, height);
    {
        Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_spritemapTilesSurface));
        
        for (index_t y{}; y < std::size(tiles) / 0x10; ++y)
            for (index_t x{}; x < 0x10; ++x)
            {
                Cairo::RefPtr<Cairo::ImageSurface> p_tileSurface(createTileSurface(tiles[y * 0x10 + x], palettes[0], false, false));
                p_context->set_source(p_tileSurface, x * 8.0, y * 8.0);
                p_context->paint();
            }
    }
}
LOG_RETHROW

void Sm::findRoomHeaders()
try
{
    Reader r(makeReader());
    for (Pointer p_roomHeader(0x8F'8000_sm); p_roomHeader < 0x90'8000_sm; ++p_roomHeader)
    {
        r.seek(p_roomHeader);
        try
        {
            knownRoomHeaders.push_back(std::make_pair(p_roomHeader, RoomHeader(r)));
        }
        catch (const std::exception&)
        {
            continue;
        }
    }
}
LOG_RETHROW

n_t Sm::decompress(byte_t* dest, n_t capacity, Reader& r)
try
{
    const byte_t* const begin_dest(dest), *const end_dest(dest + capacity);
    for (;;)
    {
        byte_t byte(r.get<byte_t>());
        if (byte == 0xFF)
            break;

        unsigned size(1), type(byte >> 5);
        if (type != 7)
            size += byte & 0x1F;
        else
        {
            type = byte >> 2 & 7;
            size += (byte & 3) << 8 | r.get<byte_t>();
        }
        if (capacity < size)
            throw std::out_of_range(LOG_INFO "Decompressed data exceeds size of buffer"s);

        switch (type)
        {
        // Direct copy
        case 0:
            r.get<byte_t>(dest, size);
            break;

        // Byte fill
        case 1:
            std::fill_n(dest, size, r.get<byte_t>());
            break;

        // Word fill
        case 2:
        {
            // Odd sizes are allowed, the LSB is copied in this case
            std::array<byte_t, 2> filler{r.get<byte_t>(), r.get<byte_t>()};
            for (index_t i{}; i < size; ++i)
                dest[i] = filler[i & 1];

            break;
        }

        // Incrementing fill
        case 3:
            std::iota(dest, dest + size, r.get<byte_t>());
            break;

        // Dictionary copy
        case 4:
        {
            // Note that std::copy/memcpy with the destination range beginning within the source range is UB,
            // and std::copy_backward would be incorrect (needs forward copying semantics)
            word_t offset(r.get<word_t>());
            if (begin_dest + offset + size > end_dest)
                throw std::out_of_range(LOG_INFO "Decompressed data exceeds size of buffer"s);

            for (index_t i{}; i < size; ++i)
                dest[i] = begin_dest[offset + i];

            break;
        }

        // Inverted dictionary copy
        case 5:
        {
            word_t offset(r.get<word_t>());
            if (begin_dest + offset + size > end_dest)
                throw std::out_of_range(LOG_INFO "Decompressed data exceeds size of buffer"s);

            for (index_t i{}; i < size; ++i)
                dest[i] = ~begin_dest[offset + i];

            break;
        }

        // Sliding dictionary copy
        case 6:
        {
            // Note that std::copy/memcpy with the destination range beginning within the source range is UB,
            // and std::copy_backward would be incorrect (needs forward copying semantics)
            byte_t offset(r.get<byte_t>());
            if (dest - offset < begin_dest)
                throw std::out_of_range(LOG_INFO "Invalid compressed data"s);

            for (index_t i{}; i < size; ++i)
                dest[i] = dest[i - offset];

            break;
        }

        // Inverted sliding dictionary copy
        case 7:
        {
            byte_t offset(r.get<byte_t>());
            if (dest - offset < begin_dest)
                throw std::out_of_range(LOG_INFO "Invalid compressed data"s);

            for (index_t i{}; i < size; ++i)
                dest[i] = ~dest[i - offset];

            break;
        }
        }

        dest += size;
    }

    return dest - begin_dest;
}
LOG_RETHROW

std::vector<Sm::byte_t> Sm::decompress(n_t maxSize, Reader& r)
try
{
    std::vector<byte_t> ret(maxSize);
    ret.resize(decompress(std::data(ret), maxSize, r));
    ret.shrink_to_fit();
    return ret;
}
LOG_RETHROW
