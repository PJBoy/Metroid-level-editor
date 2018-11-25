#include "sm.h"

#include "global.h"

#include <array>
#include <numeric>
#include <string_view>

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

void Sm::drawLevelView(Cairo::RefPtr<Cairo::Surface> p_surface) const
try
{
    if (!p_level)
        return;

    Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_surface));
    //p_context->set_antialias(Cairo::Antialias::ANTIALIAS_NONE);
    //p_context->scale(0.5, 0.5);
    p_context->set_source(p_level, 0, 0);
    p_context->paint();
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

Cairo::RefPtr<Cairo::ImageSurface> Sm::createLayerSurface(const Matrix<word_t>& layer) const
{
    Cairo::RefPtr<Cairo::ImageSurface> p_layer(Cairo::ImageSurface::create(Cairo::Format::FORMAT_ARGB32, int(layer.size_x() * 0x10), int(layer.size_y() * 0x10)));
    Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_layer));
    for (index_t y{}; y < layer.size_y(); ++y)
        for (index_t x{}; x < layer.size_x(); ++x)
        {
            const word_t i_block(levelData.layer1[y][x] & 0x3FF);
            const bool
                flip_x(levelData.layer1[y][x] >> 10 & 1),
                flip_y(levelData.layer1[y][x] >> 11 & 1);

            Cairo::RefPtr<Cairo::ImageSurface> p_block(Cairo::ImageSurface::create(Cairo::Format::FORMAT_ARGB32, 16, 16));
            {
                Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_block));
                p_context->translate(8.0, 8.0);
                p_context->scale(flip_x ? -1 : 1, flip_y ? -1 : 1);
                p_context->set_source(metatileSurfaces[i_block], -8.0, -8.0);
                p_context->paint();
            }
            p_context->set_source(p_block, x * 16.0, y * 16.0);
            p_context->paint();
        }

    return p_layer;
}

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
    p_level = Cairo::ImageSurface::create(Cairo::Format::FORMAT_ARGB32, int(levelData.n_x * 0x100), int(levelData.n_y * 0x100));
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
    if (n_x * n_y > 50)
        throw std::runtime_error("Room size "s + std::to_string(n_x) + "x"s + std::to_string(n_y) + " exceeds maximum of 50"s);

    const n_t maxScrollSize(0x10 * 0x10 * (2 + 1 + 2));
    n_t scrollSize(0x10 * 0x10 * (2 + 1));
    if (isCustomLayer2)
        scrollSize += 0x10 * 0x10 * 2;

    // Not using the room size to determine the decompress max size parameter as decompressed level data is permitted to exceed the room size
    // Using maximum room size instead
    std::vector<byte_t> decompressedData(decompress(50 * maxScrollSize + 2, r));
    if (std::size(decompressedData) < n_x * n_y * scrollSize + 2)
        throw std::runtime_error("Decompressed level data is too small for room. Expected "s + toHexString(n_x * n_y * scrollSize + 2) + " bytes, found "s + toHexString(std::size(decompressedData)) + " bytes"s);

    const word_t levelDataSize(decompressedData[0] | decompressedData[1] << 8);
    const n_t n_blocks(levelDataSize / 2);

    if (std::size(decompressedData) != n_blocks * (2 + 1) + 2 && std::size(decompressedData) != n_blocks * (2 + 1 + 2) + 2)
        throw std::runtime_error("Reported size of decompressed level data ("s + toHexString(n_blocks) + " blocks) isn't consistent with the actual size of decompressed level data ("s + toHexString(std::size(decompressedData)) + " bytes)"s);

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
LOG_RETHROW

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

void Sm::createMetatileSurface(index_t i_metatiles)
try
{
    const metatile_t& metatile(metatiles[i_metatiles]);
    Cairo::RefPtr<Cairo::ImageSurface>& p_metatileSurface(metatileSurfaces[i_metatiles]);

    auto createTileSurface([&](word_t metatilePart) -> Cairo::RefPtr<Cairo::ImageSurface>
    {
        Cairo::RefPtr<Cairo::ImageSurface> p_tileSurface(Cairo::ImageSurface::create(Cairo::Format::FORMAT_ARGB32, 8, 8));
        unsigned char* const tileSurfaceBuffer(p_tileSurface->get_data());

        const index_t i_tiles(metatilePart & 0x3FF);
        const index_t i_palettes(metatilePart >> 10 & 7);

        tile_t& tile(tiles[i_tiles]);
        palette_t& palette(bgPalettes[i_palettes]);
        for (index_t y{}; y < 8; ++y)
            for (index_t x{}; x < 8; ++x)
            {
                const index_t i_tileSurfaceBuffer((y * 8 + x) * 4);
                
                // Bitplane decoding
                index_t i_palette{};
                for (index_t i{}; i < 2; ++i)
                    for (index_t ii{}; ii < 2; ++ii)
                        i_palette |= (tile[y * 2 + i * 0x10 + ii] >> 7 - x & 1) << i * 2 + ii;

                if (i_palette == 0)
                {
                    tileSurfaceBuffer[i_tileSurfaceBuffer] = 0;
                    tileSurfaceBuffer[i_tileSurfaceBuffer + 1] = 0;
                    tileSurfaceBuffer[i_tileSurfaceBuffer + 2] = 0;
                    tileSurfaceBuffer[i_tileSurfaceBuffer + 3] = 0;
                }
                else
                {
                    word_t colour(palette[i_palette]);
                    tileSurfaceBuffer[i_tileSurfaceBuffer] = (colour >> 10 & 0x1F) * 0xFF / 0x1F;
                    tileSurfaceBuffer[i_tileSurfaceBuffer + 1] = (colour >> 5 & 0x1F) * 0xFF / 0x1F;
                    tileSurfaceBuffer[i_tileSurfaceBuffer + 2] = (colour & 0x1F) * 0xFF / 0x1F;
                    tileSurfaceBuffer[i_tileSurfaceBuffer + 3] = 0xFF;
                }
            }

        p_tileSurface->mark_dirty();
        return p_tileSurface;
    });

    p_metatileSurface = Cairo::ImageSurface::create(Cairo::Format::FORMAT_ARGB32, 16, 16);

    for (index_t y{}; y < 2; ++y)
        for (index_t x{}; x < 2; ++x)
        {
            const word_t metatilePart(metatile[y * 2 + x]);
            const bool
                bgPriority(metatilePart >> 13 & 1),
                flip_x(metatilePart >> 14 & 1),
                flip_y(metatilePart >> 15);

            // Move user-space origin to centre of tile, flip the user-space axes if necessary, plot the tile (moving the user-space origin back to the tile origin)
            Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_metatileSurface));
            p_context->translate(x * 8.0 + 4.0, y * 8.0 + 4.0);
            p_context->scale(flip_x ? -1 : 1, flip_y ? -1 : 1);
            p_context->set_source(createTileSurface(metatilePart), -4.0, -4.0);
            p_context->paint();
        }
}
LOG_RETHROW

void Sm::createMetatileSurfaces()
try
{
    for (index_t i_metatiles{}; i_metatiles < std::size(metatiles); ++i_metatiles)
        createMetatileSurface(i_metatiles);
}
LOG_RETHROW

void Sm::loadTileset(index_t i_tileset)
try
{
    decompressTileset(i_tileset);
    createMetatileSurfaces();
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
            throw std::out_of_range("Decompressed data exceeds size of buffer"s);

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
            // Note that std::copy/memcpy with overlapping ranges is UB
            word_t offset(r.get<word_t>());
            if (begin_dest + offset + size > end_dest)
                throw std::out_of_range("Decompressed data exceeds size of buffer"s);

            for (index_t i{}; i < size; ++i)
                dest[i] = begin_dest[offset + i];

            break;
        }

        // Inverted dictionary copy
        case 5:
        {
            word_t offset(r.get<word_t>());
            if (begin_dest + offset + size > end_dest)
                throw std::out_of_range("Decompressed data exceeds size of buffer"s);

            for (index_t i{}; i < size; ++i)
                dest[i] = ~begin_dest[offset + i];

            break;
        }

        // Sliding dictionary copy
        case 6:
        {
            // Note that std::copy/memcpy with overlapping ranges is UB
            byte_t offset(r.get<byte_t>());
            if (dest - offset < begin_dest)
                throw std::out_of_range("Invalid compressed data"s);

            for (index_t i{}; i < size; ++i)
                dest[i] = dest[i - offset];

            break;
        }

        // Inverted sliding dictionary copy
        case 7:
        {
            byte_t offset(r.get<byte_t>());
            if (dest - offset < begin_dest)
                throw std::out_of_range("Invalid compressed data"s);

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
