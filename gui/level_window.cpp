#include "../global.h"

import level_window;

LevelWindow::LevelWindow(Os& os, Rom& rom)
try
    : Window(os),
    p_rom(&rom)
{}
LOG_RETHROW

void LevelWindow::init(RendererFactory& rendererFactory)
try
{
    p_renderer = rendererFactory.makeWindowRenderer(*this, *p_os);

    // Temporary
    const Array2d<rom::Abgr16> romBitmap = p_rom->drawRoom({0x7'D78F});
    const array2d::Sizes romBitmapSize = romBitmap.getSizes();
    renderer::Size size;
    size.width = romBitmapSize.n_x;
    size.height = romBitmapSize.n_y;
    
    std::vector<uint8_t> bitmapData(size.width * size.height * 4);
    for (index_t y{}; y < size.height; ++y)
        for (index_t x{}; x < size.width; ++x)
        {
            const uint16_t romColour = romBitmap[y][x].colour;
            if (romColour & 0x8000)
            {
                bitmapData[(y * size.width + x) * 4] = 0;
                bitmapData[(y * size.width + x) * 4 + 1] = 0;
                bitmapData[(y * size.width + x) * 4 + 2] = 0;
                bitmapData[(y * size.width + x) * 4 + 3] = 0;
                continue;
            }

            bitmapData[(y * size.width + x) * 4]     = (romColour >> 0xA & 0x1F) * 0xFF / 0x1F;
            bitmapData[(y * size.width + x) * 4 + 1] = (romColour >> 5   & 0x1F) * 0xFF / 0x1F;
            bitmapData[(y * size.width + x) * 4 + 2] = (romColour        & 0x1F) * 0xFF / 0x1F;
            bitmapData[(y * size.width + x) * 4 + 3] = 0xFF;
        }

    p_bitmap = p_renderer->makeBitmap(bitmapData, size);
}
LOG_RETHROW

void LevelWindow::onPaint()
try
{
    std::unique_ptr<WindowDrawer> p_drawer = p_renderer->makeDrawer();
    p_drawer->drawBitmap(*p_bitmap);
}
LOG_RETHROW