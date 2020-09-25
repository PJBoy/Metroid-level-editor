#include "../../os_windows.h"

#include "../../rom.h"

#include "../../global.h"

#include <cairomm/win32_surface.h>

#include <memory>


LRESULT CALLBACK Windows::SpritemapViewer::SpritemapView::windowProcedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR lParam) noexcept
try
{
    // Window procedure reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632593

    const auto defaultHandler([&]()
    {
        return DefWindowProc(window, message, wParam, lParam);
    });

    switch (message)
    {
    default:
        return defaultHandler();

     // WM_PAINT reference: https://msdn.microsoft.com/en-us/library/dd145213
    case WM_PAINT:
    {
        // GetUpdateRect reference: https://msdn.microsoft.com/en-us/library/dd144943

        RECT updateRect;
        BOOL notEmpty(GetUpdateRect(window, &updateRect, false));
        if (notEmpty)
        {
            // BeginPaint reference: https://msdn.microsoft.com/en-us/library/dd183362
            // EndPaint reference: https://msdn.microsoft.com/en-us/library/dd162598
            // SCROLLINFO reference: https://docs.microsoft.com/en-gb/windows/desktop/api/winuser/ns-winuser-tagscrollinfo
            // GetScrollInfo reference: https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-getscrollinfo

            PAINTSTRUCT ps;
            const auto endPaint([&](HDC)
            {
                EndPaint(window, &ps);
            });
            const std::unique_ptr p_displayContext(makeUniquePtr(BeginPaint(window, &ps), endPaint));
            if (!p_displayContext)
                throw WindowsError(LOG_INFO "Failed to get display device context from BeginPaint"s);

            // Get scroll position and pass it
            /*
            SCROLLINFO si;
            si.cbSize = sizeof(si);
            si.fMask = SIF_POS;

            if (!GetScrollInfo(p_spritemapViewer->window, SB_VERT, &si))
                throw WindowsError(LOG_INFO "Could not get vertical scroll info"s);

            unsigned y(si.nPos);

            if (!GetScrollInfo(p_spritemapViewer->window, SB_HORZ, &si))
                throw WindowsError(LOG_INFO "Could not get horizontal scroll info"s);

            unsigned x(si.nPos);
            /*/
            const unsigned x{}, y{};
            //*/

            p_windows->p_rom->drawSpritemapView(Cairo::Win32Surface::create(p_displayContext.get()), x, y);
        }

        break;
    }

    // WM_HSCROLL reference: https://docs.microsoft.com/en-gb/windows/desktop/Controls/wm-hscroll
    // WM_VSCROLL reference: https://docs.microsoft.com/en-gb/windows/desktop/Controls/wm-vscroll
    case WM_HSCROLL:
    case WM_VSCROLL:
    {
        // SCROLLINFO reference: https://docs.microsoft.com/en-gb/windows/desktop/api/winuser/ns-winuser-tagscrollinfo
        // GetScrollInfo reference: https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-getscrollinfo
        // SetScrollInfo reference: https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-setscrollinfo

        int barType;
        if (message == WM_HSCROLL)
            barType = SB_HORZ;
        else
            barType = SB_VERT;

        const unsigned short request(LOWORD(wParam));

        SCROLLINFO si;
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE;
        if (!GetScrollInfo(p_window->window, barType, &si))
            throw WindowsError(LOG_INFO "Could not get " + (barType == SB_HORZ ? "horizontal"s : "vertical"s) + " scroll info"s);

        switch (request)
        {
        case SB_LINEDOWN:
            ++si.nPos;
            break;

        case SB_LINEUP:
            --si.nPos;
            break;

        case SB_PAGEDOWN:
            si.nPos += si.nPage;
            break;

        case SB_PAGEUP:
            si.nPos -= si.nPage;
            break;

        case SB_TOP:
            si.nPos = 0;
            break;

        case SB_BOTTOM:
            si.nPos = si.nMax - si.nPage + 1;
            break;

        case SB_THUMBTRACK:
            si.nPos = HIWORD(wParam);
            break;
        }

        SetScrollInfo(p_window->window, barType, &si, true);
        if (!InvalidateRect(p_windows->p_spritemapViewer->window, nullptr, true))
            throw WindowsError(LOG_INFO "Failed to invalidate spritemap viewer window"s);

        break;
    }
    }

    return 0;
}
catch (const std::exception& e)
{
    DebugFile(DebugFile::error) << LOG_INFO << e.what() << '\n';
    return DefWindowProc(window, message, wParam, lParam);
}
