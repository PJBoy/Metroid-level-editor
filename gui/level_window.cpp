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
    renderer::Size size;
    size.width = 128;
    size.height = 128;
    uint8_t data[128 * 128 * 4];
    for (index_t y{}; y < size.height; ++y)
        for (index_t x{}; x < size.width; ++x)
        {
            data[(y * size.width + x) * 4] = 0xFF * (x % 3 == 0);
            data[(y * size.width + x) * 4 + 1] = 0xFF * (x % 3 == 1);
            data[(y * size.width + x) * 4 + 2] = 0xFF * (x % 3 == 2);
            data[(y * size.width + x) * 4 + 3] = 0xFF;
        }

    p_bitmap = p_renderer->makeBitmap(data, size);
}
LOG_RETHROW

void LevelWindow::onPaint()
try
{
    std::unique_ptr<WindowDrawer> p_drawer = p_renderer->makeDrawer();
    p_drawer->drawBitmap(*p_bitmap);
}
LOG_RETHROW