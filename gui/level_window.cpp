#include "../global.h"

import level_window;

LevelWindow::LevelWindow(Os& os, Rom& rom)
try
    : Window(os),
    p_rom(&rom)
{}
LOG_RETHROW

static std::unique_ptr<renderer::Bitmap> toRendererBitmap(const Array2d<rom::Abgr16>& bitmap, WindowRenderer& renderer)
try
{
    const array2d::Sizes bitmapSize = bitmap.getSizes();
    renderer::Size size;
    size.width = bitmapSize.n_x;
    size.height = bitmapSize.n_y;
    
    std::vector<uint8_t> bitmapData(size.width * size.height * 4);
    for (index_t y{}; y < size.height; ++y)
        for (index_t x{}; x < size.width; ++x)
        {
            const uint16_t colour = bitmap[y][x].colour;
            if (colour & 0x8000)
            {
                bitmapData[(y * size.width + x) * 4] = 0;
                bitmapData[(y * size.width + x) * 4 + 1] = 0;
                bitmapData[(y * size.width + x) * 4 + 2] = 0;
                bitmapData[(y * size.width + x) * 4 + 3] = 0;
                continue;
            }

            bitmapData[(y * size.width + x) * 4]     = (colour >> 0xA & 0x1F) * 0xFF / 0x1F;
            bitmapData[(y * size.width + x) * 4 + 1] = (colour >> 5   & 0x1F) * 0xFF / 0x1F;
            bitmapData[(y * size.width + x) * 4 + 2] = (colour        & 0x1F) * 0xFF / 0x1F;
            bitmapData[(y * size.width + x) * 4 + 3] = 0xFF;
        }

    return renderer.makeBitmap(bitmapData, size);
}
LOG_RETHROW

void LevelWindow::init(RendererFactory& rendererFactory)
try
{
    p_renderer = rendererFactory.makeWindowRenderer(*this, *p_os);

    // Temporary
    //Array2d<rom::Abgr16> romBitmap = p_rom->drawRoom({0x7'D78F}); // Pre-Draygon room
    Array2d<rom::Abgr16> romBitmap = p_rom->drawRoom({0x7'AF14}); // Lava dive room
    //Array2d<rom::Abgr16> romBitmap = p_rom->drawRoom({0x7'9D19}); // Charge beam room
    
    p_bitmap = toRendererBitmap(romBitmap, *p_renderer);
}
LOG_RETHROW

void LevelWindow::onPaint()
try
{
    if (!p_bitmap)
        return;

    std::unique_ptr<WindowDrawer> p_drawer = p_renderer->makeDrawer();
    p_drawer->drawBitmap(*p_bitmap);
}
LOG_RETHROW
