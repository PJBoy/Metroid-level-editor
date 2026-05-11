#include "../../global.h"

#include <d2d1.h>

import d2d_renderer;

const D2D1_PIXEL_FORMAT pixelFormat = []
{
    // D2D1_PIXEL_FORMAT reference: https://learn.microsoft.com/en-us/windows/win32/api/dcommon/ns-dcommon-d2d1_pixel_format

    D2D1_PIXEL_FORMAT ret;
    ret.format = DXGI_FORMAT_B8G8R8A8_UNORM; // Only available format for Hwnd target
    ret.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED; // D2D1_ALPHA_MODE_STRAIGHT is not available for Hwnd target
    return ret;
}();

D2dBitmap::D2dBitmap(std::span<const uint8_t> data, Size size, ID2D1HwndRenderTarget& renderTarget)
try
    : size(std::move(size))
{
    // CreateBitmap reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nf-d2d1-id2d1rendertarget-createbitmap(d2d1_size_u_constvoid_uint32_constd2d1_bitmap_properties__id2d1bitmap)
    // D2D1_BITMAP_PROPERTIES reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/ns-d2d1-d2d1_bitmap_properties
    // BitmapProperties reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1helper/nf-d2d1helper-bitmapproperties

    // The docs don't say whether `CreateBitmap` copies the data given to it, but it also doesn't say how long the data needs to live otherwise

    const n_t rowBytes = size.width * 4;
    const D2D1_BITMAP_PROPERTIES bitmapProperties = D2D1::BitmapProperties(pixelFormat);
    long result = renderTarget.CreateBitmap(size, std::data(data), unsigned(rowBytes), bitmapProperties, &p_bitmap);
    if (result < 0)
        throw ComError(result);
}
LOG_RETHROW

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
    // BeginDraw reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nf-d2d1-id2d1rendertarget-begindraw
    
    p_renderTarget->BeginDraw();
}

D2dWindowDrawer::~D2dWindowDrawer()
{
    // EndDraw reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nf-d2d1-id2d1rendertarget-enddraw
    
    p_renderTarget->EndDraw();
}

void D2dWindowDrawer::drawBitmap(renderer::Bitmap& bitmap_in)
{
    // DrawBitmap reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nf-d2d1-id2d1rendertarget-drawbitmap(id2d1bitmap_constd2d1_rect_f__float_d2d1_bitmap_interpolation_mode_constd2d1_rect_f_)
    // D2D1_BITMAP_INTERPOLATION_MODE reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/ne-d2d1-d2d1_bitmap_interpolation_mode

    auto& bitmap = static_cast<D2dBitmap&>(bitmap_in);

    const Size bitmapSize = bitmap.getSize();
    D2D1_RECT_F sourceRectangle{};
    sourceRectangle.right = float(bitmapSize.width);
    sourceRectangle.bottom = float(bitmapSize.height);
    const D2D1_RECT_F destinationRectangle = sourceRectangle;
    const float opacity = 1;
    const D2D1_BITMAP_INTERPOLATION_MODE interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
    p_renderTarget->DrawBitmap(&bitmap.getBitmap(), destinationRectangle, opacity, interpolationMode, sourceRectangle);
}

D2dWindowRenderer::D2dWindowRenderer(ID2D1Factory& factory, const Window& window, Windows& os)
try
{
    // CreateHwndRenderTarget reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nf-d2d1-id2d1factory-createhwndrendertarget(constd2d1_render_target_properties__constd2d1_hwnd_render_target_properties__id2d1hwndrendertarget)
    // D2D1_RENDER_TARGET_PROPERTIES reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/ns-d2d1-d2d1_render_target_properties
    // D2D1_HWND_RENDER_TARGET_PROPERTIES reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/ns-d2d1-d2d1_hwnd_render_target_properties

    D2D1_RENDER_TARGET_PROPERTIES properties{};
    properties.pixelFormat = pixelFormat;
    
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProperties{};
    hwndProperties.hwnd = os.getWindowHandle(window);
    const auto [width, height] = os.getWindowSize(window);
    hwndProperties.pixelSize.width = width;
    hwndProperties.pixelSize.height = height;
    hwndProperties.presentOptions = D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS;

    long result = factory.CreateHwndRenderTarget(properties, hwndProperties, &p_renderTarget);
    if (result < 0)
        throw ComError(result);
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
    std::unique_ptr<renderer::Bitmap> ret(new D2dBitmap(data, size, *p_renderTarget));
    if (!ret)
        throw std::runtime_error(LOG_INFO "Failed to make D2dBitmap");

    return ret;
}
LOG_RETHROW

D2dRendererFactory::D2dRendererFactory()
try
{
    // D2D1CreateFactory reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nf-d2d1-d2d1createfactory-r1
    // D2D1_FACTORY_TYPE reference: https://learn.microsoft.com/en-us/windows/win32/api/d2d1/ne-d2d1-d2d1_factory_type

    const D2D1_FACTORY_TYPE factoryType = D2D1_FACTORY_TYPE_SINGLE_THREADED;
    long result = D2D1CreateFactory(factoryType, &p_factory);
    if (result < 0)
        throw ComError(result);
}
LOG_RETHROW

std::unique_ptr<WindowRenderer> D2dRendererFactory::makeWindowRenderer(const Window& window, Os& os)
try
{
    return std::unique_ptr<WindowRenderer>(new D2dWindowRenderer(*p_factory, window, static_cast<Windows&>(os)));
}
LOG_RETHROW
