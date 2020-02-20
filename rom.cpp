#include "rom.h"

#include "mf.h"
#include "mzm.h"
#include "sm.h"

#include "global.h"

Rom::Reader::Reader(std::filesystem::path filepath, index_t address /* = 0*/)
try
    : f(filepath, std::ios::binary)
{
    f.exceptions(std::ios::badbit | std::ios::failbit);
    seek(address);
}
LOG_RETHROW


Rom::Rom(std::filesystem::path filepath)
try
    : filepath(filepath)
{}
LOG_RETHROW


Rom::Reader Rom::makeReader(index_t address /* = 0*/) const
try
{
    return Rom::Reader(filepath, address);
}
LOG_RETHROW

std::ifstream Rom::makeIfstream() const
try
{
    std::ifstream f(filepath, std::ios::binary);
    f.exceptions(std::ios::badbit | std::ios::failbit);
    return f;
}
LOG_RETHROW

bool Rom::verifyRom(std::filesystem::path filepath) noexcept
try
{
    loadRom(filepath);
    return true;
}
catch (const std::exception&)
{
    return false;
}


std::unique_ptr<Rom> Rom::loadRom(std::filesystem::path filepath)
try
{
    try
    {
        return std::make_unique<Sm>(filepath);
    }
    catch (const std::exception&)
    {}

    try
    {
        return std::make_unique<Mf>(filepath);
    }
    catch (const std::exception&)
    {}

    try
    {
        return std::make_unique<Mzm>(filepath);
    }
    catch (const std::exception&)
    {}

    throw std::runtime_error("Not a valid ROM"s);
}
LOG_RETHROW

void Rom::drawLevelView(Cairo::RefPtr<Cairo::Surface> p_surface, unsigned, unsigned) const
try
{
    Cairo::RefPtr<Cairo::ImageSurface> p_tile(Cairo::ImageSurface::create(Cairo::Format::FORMAT_ARGB32, 32, 32));
    {
        Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_tile));
        p_context->set_source_rgb(0, 1, 0);
        p_context->rectangle(4, 8, 28, 24);
        p_context->fill();
    }

    Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_surface));
    p_context->set_source(p_tile, 32, 64);
    p_context->paint();
}
LOG_RETHROW

void Rom::drawSpritemapView(Cairo::RefPtr<Cairo::Surface> p_surface, unsigned, unsigned) const
try
{
    Cairo::RefPtr<Cairo::ImageSurface> p_tile(Cairo::ImageSurface::create(Cairo::Format::FORMAT_ARGB32, 32, 32));
    {
        Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_tile));
        p_context->set_source_rgb(0, 1, 0);
        p_context->rectangle(4, 8, 28, 24);
        p_context->fill();
    }

    Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_surface));
    p_context->set_source(p_tile, 32, 64);
    p_context->paint();
}
LOG_RETHROW

void Rom::drawSpritemapTilesView(Cairo::RefPtr<Cairo::Surface> p_surface, unsigned, unsigned) const
try
{
    Cairo::RefPtr<Cairo::ImageSurface> p_tile(Cairo::ImageSurface::create(Cairo::Format::FORMAT_ARGB32, 32, 32));
    {
        Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_tile));
        p_context->set_source_rgb(0, 1, 0);
        p_context->rectangle(4, 8, 28, 24);
        p_context->fill();
    }

    Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_surface));
    p_context->set_source(p_tile, 32, 64);
    p_context->paint();
}
LOG_RETHROW

auto Rom::getRoomList() const -> std::vector<RoomList>
{
    return {};
}

auto Rom::getLevelViewDimensions() const -> Dimensions
{
    return {};
}

void Rom::loadLevelData(std::vector<long>)
{}

void Rom::loadSpritemap(index_t, index_t, index_t, index_t, index_t)
{}
