module;

#include "../../global.h"

#include <d2d1.h>

export module d2d_renderer;

export import renderer;

export import os_windows; // Direct2D is Windows specific and I can't pretend otherwise

export struct Size
{
    unsigned width, height;

    Size() = default;
    Size(renderer::Size size) : width(size.width), height(size.height) {}
    Size(D2D1_SIZE_U size) : width(size.width), height(size.height) {}
    
    operator renderer::Size() const { return {width, height}; }
    operator D2D1_SIZE_U() const { return D2D1::SizeU(width, height); }
};

export class D2dBitmap : public renderer::Bitmap
{
    // ID2D1Bitmap reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nn-d2d1-id2d1bitmap

    Size size;
    ID2D1Bitmap* p_bitmap;

public:
    D2dBitmap() = default;

    // `data` must be 32bpp BGRA with A = 0xFF
    D2dBitmap(std::span<const uint8_t> data, Size size, ID2D1HwndRenderTarget& renderTarget);

    renderer::Size getSize() const override;
    
    ID2D1Bitmap& getBitmap();
};

// Offers operations that require wrapping with BeginDraw and EndDraw
export class D2dWindowDrawer : public WindowDrawer
{
    ID2D1HwndRenderTarget* p_renderTarget;

public:
    D2dWindowDrawer() = default;
    D2dWindowDrawer(D2dWindowDrawer&) = delete;
    auto operator=(D2dWindowDrawer) = delete;
    explicit D2dWindowDrawer(ID2D1HwndRenderTarget& renderTarget);
    ~D2dWindowDrawer() override;

    void drawBitmap(renderer::Bitmap& bitmap) override;
};

export class D2dWindowRenderer : public WindowRenderer
{
    // ID2D1HwndRenderTarget reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nn-d2d1-id2d1hwndrendertarget

    ID2D1HwndRenderTarget* p_renderTarget;

public:
    D2dWindowRenderer() = default;
    D2dWindowRenderer(ID2D1Factory& factory, const Window& window, Windows& os);

    std::unique_ptr<WindowDrawer> makeDrawer() override;
    std::unique_ptr<renderer::Bitmap> makeBitmap(std::span<const uint8_t> data, renderer::Size size) override;
};

export class D2dRendererFactory : public RendererFactory
{
    // ID2D1Factory reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nn-d2d1-id2d1factory

    ID2D1Factory* p_factory;

public:
    D2dRendererFactory();

    std::unique_ptr<WindowRenderer> makeWindowRenderer(const Window& window, Os& os) override;
};
