#include "../../os_windows.h"

#include "../../global.h"

import rom;
import string;

LRESULT CALLBACK Windows::SpritemapViewer::windowProcedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR lParam) noexcept
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

    // WM_COMMAND reference: https://docs.microsoft.com/en-gb/windows/win32/menurc/wm-command
    case WM_COMMAND:
    {
        if (lParam == 0)
            return defaultHandler();

        switch (HIWORD(wParam))
        {
        default:
            return defaultHandler();

        // EN_UPDATE reference: https://docs.microsoft.com/en-us/windows/win32/controls/en-update
        case EN_UPDATE:
        {
            if
            (
                   p_window->p_tilesAddressInput->isEmpty()
                || p_window->p_palettesAddressInput->isEmpty()
                || p_window->p_spritemapAddressInput->isEmpty()
                || p_window->p_tilesDestAddressInput->isEmpty()
                || p_window->p_palettesDestAddressInput->isEmpty()
            )
            {
                p_window->p_statusBar->drawText(L"");
                break;
            }

            unsigned long tilesAddress, palettesAddress, spritemapAddress, tilesDestAddress, palettesDestAddress;
            const wchar_t* statusErrorText;

            try
            {
                statusErrorText = L"Invalid value for tiles address";
                tilesAddress = p_window->p_tilesAddressInput->getValue();

                statusErrorText = L"Invalid value for palettes address";
                palettesAddress = p_window->p_palettesAddressInput->getValue();

                statusErrorText = L"Invalid value for spritemap address";
                spritemapAddress = p_window->p_spritemapAddressInput->getValue();

                statusErrorText = L"Invalid value for tiles dest address";
                tilesDestAddress = p_window->p_tilesDestAddressInput->getValue();

                statusErrorText = L"Invalid value for palettes dest address";
                palettesDestAddress = p_window->p_palettesDestAddressInput->getValue();
            }
            catch (const std::exception& e)
            {
                LOG_IGNORE(e)
                p_window->p_statusBar->drawText(statusErrorText);
                break;
            }

            try
            {
                p_window->windows.p_rom->loadSpritemap(tilesAddress, palettesAddress, spritemapAddress, tilesDestAddress, palettesDestAddress);
            }
            catch (const std::exception& e)
            {
                LOG_IGNORE(e)
                p_window->p_statusBar->drawText(toWstring(e.what()));
                break;
            }

            p_window->p_statusBar->drawText(L"Success");
            p_windows->p_spritemapViewer->invalidate();

            break;
        }
        }

        break;
    }

    // WM_ACTIVATE reference: https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-activate
    case WM_ACTIVATE:
    {
        // Save the previously active input when switching windows

        const unsigned short activationStatus(LOWORD(wParam));

        if (activationStatus == WA_INACTIVE)
            p_window->activeInput = GetFocus();

        break;
    }

    // WM_SETFOCUS reference: https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-setfocus
    case WM_SETFOCUS:
    {
        // SetFocus reference: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setfocus

        // Restore the previously active input when returning to window

        if (p_window->activeInput)
            if (!SetFocus(p_window->activeInput))
                throw WindowsError(LOG_INFO "Failed to set keyboard focus after returning to window");

        break;
    }
        
    // WM_SIZE reference: https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-size
    case WM_SIZE:
    {
        if (!p_window->p_windowLayout)
            return defaultHandler();

        if (wParam != SIZE_RESTORED && wParam != SIZE_MAXIMIZED)
            return defaultHandler();

        const int width(LOWORD(lParam)), height(HIWORD(lParam));
        p_window->p_windowLayout->resize(0, 0, width, height);

        break;
    }

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
            PAINTSTRUCT ps;
            const auto endPaint([&](HDC)
            {
                EndPaint(window, &ps);
            });
            const std::unique_ptr p_displayContext(makeUniquePtr(BeginPaint(window, &ps), endPaint));
            if (!p_displayContext)
                throw WindowsError(LOG_INFO "Failed to get display device context from BeginPaint");

            const std::wstring displayText(L"Hello, Windows!");
            if (!TextOut(p_displayContext.get(), 0, 0, std::data(displayText), static_cast<int>(std::size(displayText))))
                throw WindowsError(LOG_INFO "Failed to display text");
        }

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

void Windows::SpritemapViewer::create()
try
{
    // CreateWindowEx reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632680
    // Window style constants: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632600
    // Window extended style constants: https://msdn.microsoft.com/en-us/library/windows/desktop/ff700543

    const int x(CW_USEDEFAULT), y(CW_USEDEFAULT), width(CW_USEDEFAULT), height(CW_USEDEFAULT);
    Window::create(x, y, width, height, window);

    createChildWindows();
}
LOG_RETHROW

void Windows::SpritemapViewer::createChildWindows()
try
{
    // RECT reference: https://msdn.microsoft.com/en-us/library/windows/desktop/dd162897
    // GetClientRec reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633503
    // Edit_SetText reference: https://docs.microsoft.com/en-us/windows/win32/api/windowsx/nf-windowsx-edit_settext

    RECT rect;
    if (!GetClientRect(window, &rect))
        throw WindowsError(LOG_INFO "Failed to get size of client area of spritemap viewer window"s);

    p_statusBar                = std::make_unique<StatusBarWindow>(windows);
    p_spritemapView            = std::make_unique<SpritemapView>(windows);
    p_tilesAddressLabel        = std::make_unique<TilesAddressLabel>(windows);
    p_tilesAddressInput        = std::make_unique<TilesAddressInput>(windows);
    p_tilesDestAddressLabel    = std::make_unique<TilesDestAddressLabel>(windows);
    p_tilesDestAddressInput    = std::make_unique<TilesDestAddressInput>(windows);
    p_palettesAddressLabel     = std::make_unique<PaletteAddressLabel>(windows);
    p_palettesAddressInput     = std::make_unique<PaletteAddressInput>(windows);
    p_palettesDestAddressLabel = std::make_unique<PaletteDestAddressLabel>(windows);
    p_palettesDestAddressInput = std::make_unique<PaletteDestAddressInput>(windows);
    p_spritemapAddressLabel    = std::make_unique<SpritemapAddressLabel>(windows);
    p_spritemapAddressInput    = std::make_unique<SpritemapAddressInput>(windows);
    p_spritemapTilesView       = std::make_unique<SpritemapTilesView>(windows);

    // Status bar is exluded from the window layout because its size is managed automatically
    p_statusBar->create({}, {}, {}, {}, window);
    rect.bottom -= p_statusBar->getHeight();

    p_windowLayout = WindowRow::make
    ({
        {1./3, WindowColumn::make
        ({
            {16, WindowRow::make // 16 is the height of the monospace font created in `Windows::init()`
            ({
                {96, p_tilesAddressLabel.get()},
                {96, p_tilesAddressInput.get()},
                {20, p_tilesDestAddressLabel.get()},
                {96, p_tilesDestAddressInput.get()}
            })},
            {16, WindowRow::make
            ({
                {96, p_palettesAddressLabel.get()},
                {96, p_palettesAddressInput.get()},
                {20, p_palettesDestAddressLabel.get()},
                {96, p_palettesDestAddressInput.get()}
            })},
            {16, WindowRow::make
            ({
                {96, p_spritemapAddressLabel.get()},
                {96, p_spritemapAddressInput.get()}
            })},
            {0x200, p_spritemapTilesView.get()}
        }, 1)},
        {2./3, p_spritemapView.get()}
    });

    p_windowLayout->create(0, 0, rect.right, rect.bottom, window);
    if (!SetFocus(p_tilesAddressInput->window))
        throw WindowsError(LOG_INFO "Failed to set keyboard focus after creating spritemap window");

    // Temp defaults:
    Edit_SetText(p_tilesAddressInput->window,        L"ABCC00");
    Edit_SetText(p_tilesDestAddressInput->window,    L"7000");
    Edit_SetText(p_palettesAddressInput->window,     L"A78687");
    Edit_SetText(p_palettesDestAddressInput->window, L"80");
    Edit_SetText(p_spritemapAddressInput->window,    L"A7A5DF");
}
LOG_RETHROW
