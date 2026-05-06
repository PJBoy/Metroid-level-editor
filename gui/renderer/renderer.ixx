module;

#include "../../global.h"

export module renderer;

export import os;
export import window;

export namespace renderer
{
struct Size
{
    unsigned width, height;
};

class Bitmap
{
public:
    virtual ~Bitmap() = default;

    virtual Size getSize() const = 0;
};
}

export class WindowDrawer
{
public:
    virtual ~WindowDrawer() = default;

    virtual void drawBitmap(renderer::Bitmap& bitmap) = 0;
};

export class WindowRenderer
{
public:
    virtual ~WindowRenderer() = default;

    virtual std::unique_ptr<WindowDrawer> makeDrawer() = 0;

    // `data` must be 32bpp BGRA with A = 0xFF
    virtual std::unique_ptr<renderer::Bitmap> makeBitmap(std::span<const uint8_t> data, renderer::Size size) = 0;
};

export class RendererFactory
{
public:
    virtual std::unique_ptr<WindowRenderer> makeWindowRenderer(const Window& window, Os& os) = 0;
};
