#include "../../global.h"

#include <d2d1.h>

import d2d_renderer;

D2dBitmap::D2dBitmap(std::span<const uint8_t> data, Size size, ID2D1HwndRenderTarget& renderTarget)
    : size(std::move(size))
{
    // CreateBitmap reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nf-d2d1-id2d1rendertarget-createbitmap(d2d1_size_u_constvoid_uint32_constd2d1_bitmap_properties__id2d1bitmap)

    const n_t rowBytes = size.width * 4;
    D2D1_PIXEL_FORMAT pixelFormat;
    pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM; // Only available format for Hwnd target
    pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED; // D2D1_ALPHA_MODE_STRAIGHT is not available for Hwnd target
    const D2D1_BITMAP_PROPERTIES bitmapProperties = D2D1::BitmapProperties(pixelFormat);

    // todo error check
    renderTarget.CreateBitmap(size, std::data(data), rowBytes, bitmapProperties, &p_bitmap);
}

renderer::Size D2dBitmap::getSize() const
{
    return size;
}

ID2D1Bitmap& D2dBitmap::getBitmap()
{
    return *p_bitmap;
}

D2dWindowDrawer::D2dWindowDrawer(ID2D1HwndRenderTarget& renderTarget)
    : p_renderTarget(&renderTarget)
{
    p_renderTarget->BeginDraw();
}

D2dWindowDrawer::~D2dWindowDrawer()
{
    p_renderTarget->EndDraw();
}

void D2dWindowDrawer::drawBitmap(renderer::Bitmap& bitmap_in)
{
    // DrawBitmap reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nf-d2d1-id2d1rendertarget-drawbitmap(id2d1bitmap_constd2d1_rect_f__float_d2d1_bitmap_interpolation_mode_constd2d1_rect_f_)

    auto& bitmap = static_cast<D2dBitmap&>(bitmap_in);

    const Size bitmapSize = bitmap.getSize();
    D2D1_RECT_F sourceRectangle{};
    sourceRectangle.right = bitmapSize.width;
    sourceRectangle.bottom = bitmapSize.height;
    const D2D1_RECT_F destinationRectangle = sourceRectangle;
    const float opacity = 1;
    const D2D1_BITMAP_INTERPOLATION_MODE interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
    p_renderTarget->DrawBitmap(&bitmap.getBitmap(), destinationRectangle, opacity, interpolationMode, sourceRectangle);
}

D2dWindowRenderer::D2dWindowRenderer(ID2D1Factory& factory, const Window& window, Windows& os)
try
{
    // CreateHwndRenderTarget reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nf-d2d1-id2d1factory-createhwndrendertarget(constd2d1_render_target_properties__constd2d1_hwnd_render_target_properties__id2d1hwndrendertarget)
    // RenderTargetProperties reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/ns-d2d1-d2d1_render_target_properties
    // HwndRenderTargetProperties reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/ns-d2d1-d2d1_hwnd_render_target_properties

    // todo error check
    const auto [width, height] = os.getWindowSize(window);
    HWND handle = os.getWindowHandle(window);
    factory.CreateHwndRenderTarget
    (
        D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
        D2D1::HwndRenderTargetProperties(handle, D2D1::SizeU(width, height), D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS),
        &p_renderTarget
    );
}
LOG_RETHROW

std::unique_ptr<WindowDrawer> D2dWindowRenderer::makeDrawer()
try
{
    return std::unique_ptr<WindowDrawer>(new D2dWindowDrawer(*p_renderTarget));
}
LOG_RETHROW

std::unique_ptr<renderer::Bitmap> D2dWindowRenderer::makeBitmap(std::span<const uint8_t> data, renderer::Size size)
try
{
    return std::unique_ptr<renderer::Bitmap>(new D2dBitmap(data, size, *p_renderTarget));
}
LOG_RETHROW

D2dRendererFactory::D2dRendererFactory()
try
{
    // D2D1CreateFactory reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nf-d2d1-d2d1createfactory-r1

    // todo error check
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &p_factory);
}
LOG_RETHROW

std::unique_ptr<WindowRenderer> D2dRendererFactory::makeWindowRenderer(const Window& window, Os& os)
try
{
    return std::unique_ptr<WindowRenderer>(new D2dWindowRenderer(*p_factory, window, static_cast<Windows&>(os)));
}
LOG_RETHROW
