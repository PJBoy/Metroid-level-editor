#include "../../os_windows.h"

#include "../../rom.h"

#include "../../global.h"

#include <memory>


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
                   p_spritemapViewer->p_tilesAddressInput->isEmpty()
                || p_spritemapViewer->p_palettesAddressInput->isEmpty()
                || p_spritemapViewer->p_spritemapAddressInput->isEmpty()
                || p_spritemapViewer->p_tilesDestAddressInput->isEmpty()
                || p_spritemapViewer->p_palettesDestAddressInput->isEmpty()
            )
            {
                p_spritemapViewer->p_statusBar->drawText(L"");
                break;
            }

            unsigned long tilesAddress, palettesAddress, spritemapAddress, tilesDestAddress, palettesDestAddress;

            try
            {
                tilesAddress = p_spritemapViewer->p_tilesAddressInput->getValue();
            }
            catch (const std::exception& e)
            {
                LOG_IGNORE(e)
                p_spritemapViewer->p_statusBar->drawText(L"Invalid value for tiles address");
                break;
            }

            try
            {
                palettesAddress = p_spritemapViewer->p_palettesAddressInput->getValue();
            }
            catch (const std::exception& e)
            {
                LOG_IGNORE(e)
                p_spritemapViewer->p_statusBar->drawText(L"Invalid value for palettes address");
                break;
            }

            try
            {
                spritemapAddress = p_spritemapViewer->p_spritemapAddressInput->getValue();
            }
            catch (const std::exception& e)
            {
                LOG_IGNORE(e)
                p_spritemapViewer->p_statusBar->drawText(L"Invalid value for spritemap address");
                break;
            }

            try
            {
                tilesDestAddress = p_spritemapViewer->p_tilesDestAddressInput->getValue();
            }
            catch (const std::exception& e)
            {
                LOG_IGNORE(e)
                p_spritemapViewer->p_statusBar->drawText(L"Invalid value for tiles dest address");
                break;
            }

            try
            {
                palettesDestAddress = p_spritemapViewer->p_palettesDestAddressInput->getValue();
            }
            catch (const std::exception& e)
            {
                LOG_IGNORE(e)
                p_spritemapViewer->p_statusBar->drawText(L"Invalid value for palettes dest address");
                break;
            }

            try
            {
                p_spritemapViewer->windows.p_rom->loadSpritemap(tilesAddress, palettesAddress, spritemapAddress, tilesDestAddress, palettesDestAddress);
            }
            catch (const std::exception& e)
            {
                LOG_IGNORE(e)
                p_spritemapViewer->p_statusBar->drawText(toWstring(e.what()));
                break;
            }

            p_spritemapViewer->p_statusBar->drawText(L"Success");
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
            p_spritemapViewer->activeInput = GetFocus();

        break;
    }

    // WM_SETFOCUS reference: https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-setfocus
    case WM_SETFOCUS:
    {
        // SetFocus reference: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setfocus

        // Restore the previously active input when returning to window

        if (p_spritemapViewer->activeInput)
            if (!SetFocus(p_spritemapViewer->activeInput))
                throw WindowsError(LOG_INFO "Failed to set keyboard focus after returning to window");

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

Windows::SpritemapViewer::SpritemapViewer(Windows& windows)
try
    : Window(windows)
{
    p_spritemapViewer = this;
}
LOG_RETHROW

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
    // INITCOMMONCONTROLSEX reference: https://docs.microsoft.com/en-gb/windows/desktop/api/commctrl/ns-commctrl-taginitcommoncontrolsex
    // GetClientRec reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633503
    // InitCommonControlsEx reference: https://docs.microsoft.com/en-gb/windows/desktop/api/commctrl/nf-commctrl-initcommoncontrolsex
    // Edit_SetText reference: https://docs.microsoft.com/en-us/windows/win32/api/windowsx/nf-windowsx-edit_settext

    INITCOMMONCONTROLSEX iccs{};
    iccs.dwSize = sizeof(iccs);
    iccs.dwICC = ICC_TREEVIEW_CLASSES;
    if (!InitCommonControlsEx(&iccs))
        throw std::runtime_error(LOG_INFO "Could not initialise common controls"s);

    RECT rect;
    if (!GetClientRect(window, &rect))
        throw WindowsError(LOG_INFO "Failed to get size of client area of spritemap viewer window"s);

    if (!p_statusBar)
        p_statusBar = std::make_unique<StatusBarWindow>(windows);

    if (!p_spritemapView)
        p_spritemapView = std::make_unique<SpritemapView>(windows);

    if (!p_tilesAddressLabel)
        p_tilesAddressLabel = std::make_unique<TilesAddressLabel>(windows);

    if (!p_tilesAddressInput)
        p_tilesAddressInput = std::make_unique<TilesAddressInput>(windows);

    if (!p_tilesDestAddressLabel)
        p_tilesDestAddressLabel = std::make_unique<TilesDestAddressLabel>(windows);

    if (!p_tilesDestAddressInput)
        p_tilesDestAddressInput = std::make_unique<TilesDestAddressInput>(windows);

    if (!p_palettesAddressLabel)
        p_palettesAddressLabel = std::make_unique<PaletteAddressLabel>(windows);

    if (!p_palettesAddressInput)
        p_palettesAddressInput = std::make_unique<PaletteAddressInput>(windows);

    if (!p_palettesDestAddressLabel)
        p_palettesDestAddressLabel = std::make_unique<PaletteDestAddressLabel>(windows);

    if (!p_palettesDestAddressInput)
        p_palettesDestAddressInput = std::make_unique<PaletteDestAddressInput>(windows);

    if (!p_spritemapAddressLabel)
        p_spritemapAddressLabel = std::make_unique<SpritemapAddressLabel>(windows);

    if (!p_spritemapAddressInput)
        p_spritemapAddressInput = std::make_unique<SpritemapAddressInput>(windows);

    if (!p_spritemapTilesView)
        p_spritemapTilesView = std::make_unique<SpritemapTilesView>(windows);

    // Status bar is exluded from the window layout because its size is managed automatically
    p_statusBar->create({}, {}, {}, {}, window);
    rect.bottom -= p_statusBar->getHeight();

    WindowRow windowLayout
    ({
        {1./3, WindowColumn::make
        ({
            {19, WindowRow::make
            ({
                {96, p_tilesAddressLabel.get()},
                {96, WindowColumn::make({{16, p_tilesAddressInput.get()}})}, // edit control seems to have a minimum height, but the text does get cropped to the specified height, ugh
                {20, p_tilesDestAddressLabel.get()},
                {96, WindowColumn::make({{16, p_tilesDestAddressInput.get()}})}
            })},
            {19, WindowRow::make
            ({
                {96, p_palettesAddressLabel.get()},
                {96, WindowColumn::make({{16, p_palettesAddressInput.get()}})},
                {20, p_palettesDestAddressLabel.get()},
                {96, WindowColumn::make({{16, p_palettesDestAddressInput.get()}})}
            })},
            {19, WindowRow::make
            ({
                {96, p_spritemapAddressLabel.get()},
                {96, WindowColumn::make({{16, p_spritemapAddressInput.get()}})}
            })},
            {0x200, p_spritemapTilesView.get()}
        }, 1)},
        {2./3, p_spritemapView.get()}
    });

    windowLayout.create(window, rect.right, rect.bottom);
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


// LabelWindows
Windows::SpritemapViewer::TilesAddressLabel::TilesAddressLabel(Windows& windows)
try
    : LabelWindow(windows)
{
    p_tilesAddressLabel = this;
}
LOG_RETHROW

Windows::SpritemapViewer::PaletteAddressLabel::PaletteAddressLabel(Windows& windows)
try
    : LabelWindow(windows)
{
    p_paletteAddressLabel = this;
}
LOG_RETHROW

Windows::SpritemapViewer::SpritemapAddressLabel::SpritemapAddressLabel(Windows& windows)
try
    : LabelWindow(windows)
{
    p_spritemapAddressLabel = this;
}
LOG_RETHROW

Windows::SpritemapViewer::TilesDestAddressLabel::TilesDestAddressLabel(Windows& windows)
try
    : LabelWindow(windows)
{
    p_tilesDestAddressLabel = this;
}
LOG_RETHROW

Windows::SpritemapViewer::PaletteDestAddressLabel::PaletteDestAddressLabel(Windows& windows)
try
    : LabelWindow(windows)
{
    p_paletteDestAddressLabel = this;
}
LOG_RETHROW


// AddressEditWindows
Windows::SpritemapViewer::TilesAddressInput::TilesAddressInput(Windows& windows)
try
    : AddressEditWindow(windows)
{
    p_tilesAddressInput = this;
}
LOG_RETHROW

Windows::SpritemapViewer::PaletteAddressInput::PaletteAddressInput(Windows& windows)
try
    : AddressEditWindow(windows)
{
    p_paletteAddressInput = this;
}
LOG_RETHROW

Windows::SpritemapViewer::SpritemapAddressInput::SpritemapAddressInput(Windows& windows)
try
    : AddressEditWindow(windows)
{
    p_spritemapAddressInput = this;
}
LOG_RETHROW

Windows::SpritemapViewer::TilesDestAddressInput::TilesDestAddressInput(Windows& windows)
try
    : AddressEditWindow(windows)
{
    p_tilesDestAddressInput = this;
}
LOG_RETHROW

Windows::SpritemapViewer::PaletteDestAddressInput::PaletteDestAddressInput(Windows& windows)
try
    : AddressEditWindow(windows)
{
    p_paletteDestAddressInput = this;
}
LOG_RETHROW
