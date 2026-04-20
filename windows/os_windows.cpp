// Windows API reference: https://learn.microsoft.com/en-us/previous-versions/aa383749(v=vs.85)
// Windows API reference - functions in alphabetical order: https://learn.microsoft.com/en-us/previous-versions/aa383688(v=vs.85)
// Windows API reference - functions by category: https://learn.microsoft.com/en-us/previous-versions/aa383686(v=vs.85)
// Windows API reference - data types: https://learn.microsoft.com/en-gb/windows/win32/winprog/windows-data-types
// Other Windows API reference: https://learn.microsoft.com/en-gb/windows/win32/apiindex/windows-api-list
// Window article: https://learn.microsoft.com/en-gb/windows/win32/winmsg/using-windows
// Controls reference: https://learn.microsoft.com/en-us/windows/win32/controls/window-controls
// Control class names: https://learn.microsoft.com/en-us/windows/win32/controls/common-control-window-classes

// TODO: enable strict mode

#include "../global.h"

#define OEMRESOURCE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <windowsx.h>

#include <cstdio> // for std{in,out,err}
#include <cstdlib> // for EXIT_SUCCESS

import os_windows;

import string;


static std::map<HWND, Window*> windowMap;

static long CALLBACK vectoredHandler(EXCEPTION_POINTERS* p_e) noexcept
{
    // VectoredHandler reference: https://learn.microsoft.com/en-gb/windows/win32/api/winnt/nc-winnt-pvectored_exception_handler

    // Ignore C++ exceptions (according to https://support.microsoft.com/en-gb/help/185294/prb-exception-code-0xe06d7363-when-calling-win32-seh-apis )
    if (p_e->ExceptionRecord->ExceptionCode >> 0x18 == 0xE0)
        return EXCEPTION_CONTINUE_SEARCH; // Note that returning EXCEPTION_CONTINUE_EXECUTION would cancel the thrown C++ exception, essentially NOPing it

    // These are informational according to https://stackoverflow.com/questions/12298406/how-to-treat-0x40010006-exception-in-vectored-exception-handler
    if (p_e->ExceptionRecord->ExceptionCode < 0x80000001)
        return EXCEPTION_CONTINUE_SEARCH;

    DebugFile debug(DebugFile::error);
    debug << LOG_INFO << std::hex
        << (p_e->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE ? "Non-continuable" : "Continuable") << " Windows exception thrown:\n"
        << "Exception code: " << p_e->ExceptionRecord->ExceptionCode << " - ";

    switch (p_e->ExceptionRecord->ExceptionCode)
    {
    default:
        debug << "Unknown exception.\n";
        break;

    case EXCEPTION_ACCESS_VIOLATION:
        debug << "EXCEPTION_ACCESS_VIOLATION: The thread tried to read from or write to a virtual address for which it does not have the appropriate access.\n";
        break;

    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        debug << "EXCEPTION_ARRAY_BOUNDS_EXCEEDED: The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking.\n";
        break;

    case EXCEPTION_BREAKPOINT:
        debug << "EXCEPTION_BREAKPOINT: A breakpoint was encountered.\n";
        break;

    case EXCEPTION_DATATYPE_MISALIGNMENT:
        debug << "EXCEPTION_DATATYPE_MISALIGNMENT: The thread tried to read or write data that is misaligned on hardware that does not provide alignment.\n";
        break;

    case EXCEPTION_FLT_DENORMAL_OPERAND:
        debug << "EXCEPTION_FLT_DENORMAL_OPERAND: One of the operands in a floating-point operation is denormal.\n";
        break;

    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        debug << "EXCEPTION_FLT_DIVIDE_BY_ZERO: The thread tried to divide a floating-point value by a floating-point divisor of zero.\n";
        break;

    case EXCEPTION_FLT_INEXACT_RESULT:
        debug << "EXCEPTION_FLT_INEXACT_RESULT: The result of a floating-point operation cannot be represented exactly as a decimal fraction.\n";
        break;

    case EXCEPTION_FLT_INVALID_OPERATION:
        debug << "EXCEPTION_FLT_INVALID_OPERATION: This exception represents any floating-point exception not included in this list.\n";
        break;

    case EXCEPTION_FLT_OVERFLOW:
        debug << "EXCEPTION_FLT_OVERFLOW: The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type.\n";
        break;

    case EXCEPTION_FLT_STACK_CHECK:
        debug << "EXCEPTION_FLT_STACK_CHECK: The stack overflowed or underflowed as the result of a floating-point operation.\n";
        break;

    case EXCEPTION_FLT_UNDERFLOW:
        debug << "EXCEPTION_FLT_UNDERFLOW: The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.\n";
        break;

    case EXCEPTION_ILLEGAL_INSTRUCTION:
        debug << "EXCEPTION_ILLEGAL_INSTRUCTION: The thread tried to execute an invalid instruction.\n";
        break;

    case EXCEPTION_IN_PAGE_ERROR:
        debug << "EXCEPTION_IN_PAGE_ERROR: The thread tried to access a page that was not present, and the system was unable to load the page.\n";
        break;

    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        debug << "EXCEPTION_INT_DIVIDE_BY_ZERO: The thread tried to divide an integer value by an integer divisor of zero.\n";
        break;

    case EXCEPTION_INT_OVERFLOW:
        debug << "EXCEPTION_INT_OVERFLOW: The result of an integer operation caused a carry out of the most significant bit of the result.\n";
        break;

    case EXCEPTION_INVALID_DISPOSITION:
        debug << "EXCEPTION_INVALID_DISPOSITION: An exception handler returned an invalid disposition to the exception dispatcher.\n";
        break;

    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        debug << "EXCEPTION_NONCONTINUABLE_EXCEPTION: The thread tried to continue execution after a noncontinuable exception occurred.\n";
        break;

    case EXCEPTION_PRIV_INSTRUCTION:
        debug << "EXCEPTION_PRIV_INSTRUCTION: The thread tried to execute an instruction whose operation is not allowed in the current machine mode.\n";
        break;

    case EXCEPTION_SINGLE_STEP:
        debug << "EXCEPTION_SINGLE_STEP: A trace trap or other single-instruction mechanism signaled that one instruction has been executed.\n";
        break;

    case EXCEPTION_STACK_OVERFLOW:
        debug << "EXCEPTION_STACK_OVERFLOW: The thread used up its stack.\n";
        break;
    }

    debug
        << "Exception address: " << p_e->ExceptionRecord->ExceptionAddress << '\n'
        << '\n';

    if (p_e->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || p_e->ExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
    {
        const auto
            readWrite(p_e->ExceptionRecord->ExceptionInformation[0]),
            address(p_e->ExceptionRecord->ExceptionInformation[1]);

        debug << "Read/write flags: " << readWrite << " - ";
        switch (readWrite)
        {
        default:
            debug << "unknown.\n";

        case 0:
            debug << "thread attempted to read inaccessible data.\n";
            break;

        case 1:
            debug << "thread attempted to write inaccessible data.\n";
            break;

        case 8:
            debug << "thread caused a user-mode data execution prevention violation.\n";
            break;
        }

        debug << "Virtual address: " << address << '\n';
        if (p_e->ExceptionRecord->ExceptionCode == EXCEPTION_IN_PAGE_ERROR)
        {
            const auto ntstatus(p_e->ExceptionRecord->ExceptionInformation[2]);
            debug << "NTSTATUS code: " << ntstatus << '\n';
        }
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

Windows::Windows(HINSTANCE instance) noexcept
    : instance(instance)
{
    // No logging allowed here

    // SetErrorMode reference: https://learn.microsoft.com/en-gb/windows/win32/api/errhandlingapi/nf-errhandlingapi-seterrormode

    // Do not display the Windows error reporting dialog
    SetErrorMode(SEM_NOGPFAULTERRORBOX);
}

static bool isChildWindow(HWND windowHandle)
{
    // GetAncestor reference: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getancestor
    // GetDesktopWindow reference: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdesktopwindow
    
    HWND parentHandle(GetAncestor(windowHandle, GA_PARENT));
    return parentHandle && parentHandle != GetDesktopWindow();
}

static Menu& findMenu(Menu& menu, HMENU rootMenuHandle, HMENU targetMenuHandle)
try
{
    std::stack<std::pair<Menu*, HMENU>> menus;
    menus.push({&menu, rootMenuHandle});
    do
    {
        auto [p_menu, menuHandle](std::move(menus.top()));
        if (menuHandle == targetMenuHandle)
            return *p_menu;

        menus.pop();
        const n_t n_entries(std::size(p_menu->entries));
        for (index_t i_entry{}; i_entry < n_entries; ++i_entry)
        {
            MenuEntry& entry(p_menu->entries[i_entry]);
            if (!entry.isSubmenu())
                continue;

            HMENU const submenuHandle(GetSubMenu(menuHandle, int(i_entry)));
            if (!submenuHandle)
                throw WindowsError(LOG_INFO "GetSubMenu failed");

            menus.push({&entry.asSubmenu(), submenuHandle});
        }
    } while (!menus.empty());

    throw std::runtime_error(LOG_INFO "Could not find menu corresponding to menu command");
}
LOG_RETHROW

static void handleMenuCommand(HWND windowHandle, HMENU targetMenuHandle, index_t i_targetMenuEntry)
try
{
    // GetMenu reference: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmenu
    // GetSubMenu reference: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getsubmenu

    if (isChildWindow(windowHandle))
        throw WindowsError(LOG_INFO "Menu command received for child window");

    Window& window(*windowMap[windowHandle]);
    if (!window.menu)
        throw WindowsError(LOG_INFO "Menu command received for window that has no menu");

    HMENU const menuHandle(GetMenu(windowHandle));
    if (!menuHandle)
        throw WindowsError(LOG_INFO "GetMenu failed");

    Menu& menu(findMenu(*window.menu, menuHandle, targetMenuHandle));
    if (i_targetMenuEntry >= std::size(menu.entries))
        throw std::runtime_error(LOG_INFO "Menu command received with invalid index " + std::to_string(i_targetMenuEntry));

    MenuEntry& menuEntry(menu.entries[i_targetMenuEntry]);
    if (menuEntry.isSubmenu())
        throw std::runtime_error(LOG_INFO "Menu command received for submenu");

    menuEntry.asItem().action(window);
}
LOG_RETHROW

static LRESULT CALLBACK windowProcedure(HWND windowHandle, unsigned message, std::uintptr_t wParam, LONG_PTR lParam) noexcept
try
{
    // Window procedure reference: https://learn.microsoft.com/en-gb/windows/win32/winmsg/window-procedures

    const auto defaultHandler([&]()
    {
        return DefWindowProc(windowHandle, message, wParam, lParam);
    });

    switch (message)
    {
    default:
        return defaultHandler();

    // WM_DESTROY reference: https://learn.microsoft.com/en-gb/windows/win32/winmsg/wm-destroy
    case WM_DESTROY:
    {
        windowMap[windowHandle]->onDestroy();
        
        break;
    }

    // WM_COMMAND reference: https://learn.microsoft.com/en-gb/windows/win32/menurc/wm-command
    case WM_COMMAND:
    {
        const bool isControl(lParam != 0);
        const unsigned short id(LOWORD(wParam));

        if (isControl)
            return defaultHandler();
        
        const bool isAccelerator(HIWORD(wParam) != 0);
        if (!isAccelerator)
            throw std::runtime_error(LOG_INFO "Received menu command in WM_COMMAND message");

        break;
    }

    // WM_MENUCOMMAND reference: https://learn.microsoft.com/en-us/windows/win32/menurc/wm-menucommand
    case WM_MENUCOMMAND:
    {
        const index_t i_menuItem(wParam);
        const auto menuHandle(reinterpret_cast<HMENU>(lParam));
        handleMenuCommand(windowHandle, menuHandle, i_menuItem);
    }

    // WM_PAINT reference: https://learn.microsoft.com/en-gb/windows/win32/gdi/wm-paint
    case WM_PAINT:
    {
        // GetUpdateRect reference: https://learn.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-getupdaterect
        RECT updateRect;
        BOOL notEmpty(GetUpdateRect(windowHandle, &updateRect, false));
        if (notEmpty)
        {
            // BeginPaint reference: https://learn.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-beginpaint
            // EndPaint reference: https://learn.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-endpaint
            PAINTSTRUCT ps;
            const auto endPaint([&](HDC)
            {
                EndPaint(windowHandle, &ps);
            });
            const std::unique_ptr p_displayContext(makeUniquePtr(BeginPaint(windowHandle, &ps), endPaint));
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
    return DefWindowProc(windowHandle, message, wParam, lParam);
}

static HICON loadIcon()
try
{
    // LoadImage reference: https://learn.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-loadimagew
    auto ret(static_cast<HICON>(LoadImage({}, IDI_APPLICATION, IMAGE_ICON, {}, {}, LR_DEFAULTSIZE | LR_SHARED)));
    if (!ret)
        throw WindowsError(LOG_INFO "Failed to load icon");

    return ret;
}
LOG_RETHROW

static HICON loadSmallIcon()
try
{
    // LoadImage reference: https://learn.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-loadimagew
    auto ret(static_cast<HICON>(LoadImage({}, IDI_APPLICATION, IMAGE_ICON, SM_CXSMICON, SM_CYSMICON, LR_SHARED)));
    if (!ret)
        throw WindowsError(LOG_INFO "Failed to load small icon");

    return ret;
}
LOG_RETHROW

static HCURSOR loadCursor()
try
{
    // LoadImage reference: https://learn.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-loadimagew
    auto ret(static_cast<HCURSOR>(LoadImage({}, IDC_ARROW, IMAGE_CURSOR, {}, {}, LR_SHARED | LR_DEFAULTSIZE)));
    if (!ret)
        throw WindowsError(LOG_INFO "Failed to load cursor");

    return ret;
}
LOG_RETHROW

static HBRUSH loadBackground()
try
{
    // CreateSolidBrush reference: https://learn.microsoft.com/en-gb/windows/win32/api/wingdi/nf-wingdi-createsolidbrush
    HBRUSH ret(CreateSolidBrush(0x000000));
    if (!ret)
        throw WindowsError(LOG_INFO "Failed to create background brush");

    return ret;
}
LOG_RETHROW

static void registerClass(HINSTANCE instance, const wchar_t* className)
try
{
    // Window class article: https://learn.microsoft.com/en-gb/windows/win32/winmsg/about-window-classes
    // WNDCLASSEXW reference: https://learn.microsoft.com/en-gb/windows/win32/api/winuser/ns-winuser-wndclassexw
    // Window class style constants: https://learn.microsoft.com/en-gb/windows/win32/winmsg/window-class-styles
    // RegisterClassEx reference: https://learn.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-registerclassexa

    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW; // Redraw the entire window if a movement or size adjustment changes the width or height of the client area
    wcex.lpfnWndProc = windowProcedure;
    wcex.hInstance = instance;
    wcex.hIcon = loadIcon();
    wcex.hCursor = loadCursor();
    wcex.hbrBackground = loadBackground();
    wcex.lpszClassName = className;
    wcex.hIconSm = loadSmallIcon();

    ATOM registeredClass(RegisterClassEx(&wcex));
    if (!registeredClass)
        throw WindowsError(LOG_INFO "Failed to register "s + toString(className) + " class");
}
LOG_RETHROW

static HWND createWindow(HINSTANCE instance, const wchar_t* className, const wchar_t* title, int cmdShow, HMENU menu)
try
{
    // CreateWindowEx reference: https://learn.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-createwindowexw
    // Window style constants: https://learn.microsoft.com/en-gb/windows/win32/winmsg/window-styles
    // Window extended style constants: https://learn.microsoft.com/en-gb/windows/win32/winmsg/extended-window-styles

    // style & WS_VISIBLE shows the window after creation.
    // style & WS_TILEDWINDOW (a.k.a. WS_OVERLAPPEDWINDOW) gives the window a title bar (a.k.a. caption), min/max boxes, system menu, menu and sizing border (a.k.a. thick frame)
    // When x == CW_USEDEFAULT and y != CW_USEDEFAULT, y is used as the cmdShow parameter

    const unsigned long exStyle{};
    const unsigned long style(WS_TILEDWINDOW | WS_VISIBLE);
    const int x(CW_USEDEFAULT);
    const int y(cmdShow);
    const int width(CW_USEDEFAULT);
    const int height(CW_USEDEFAULT);
    HWND const windowParent{};
    void* const param{};
    HWND const window(CreateWindowEx(exStyle, className, title, style, x, y, width, height, windowParent, menu, instance, param));
    if (!window)
        throw WindowsError(LOG_INFO "Failed to create "s + toString(title) + " window");

    return window;
}
LOG_RETHROW

static HMENU createMenu(const Menu& menu)
try
{
    // CreateMenu reference: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createmenu
    // InsertMenuItem reference: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-insertmenuitemw

    HMENU const menuHandle(CreateMenu());
    if (!menuHandle)
        throw WindowsError(LOG_INFO "Failed to create menu");
    
    index_t i_insertion{};
    for (const MenuEntry& menuEntry : menu.entries)
    {
        std::wstring itemText(toWstring(menuEntry.text));

        MENUITEMINFO info{};
        info.cbSize = sizeof(info);
        info.fMask = MIIM_STATE | MIIM_STRING | MIIM_SUBMENU;
        if (menuEntry.isDisabled)
            info.fState = MFS_DISABLED;

        if (menuEntry.isSubmenu())
        {
            info.hSubMenu = createMenu(menuEntry.asSubmenu());
        }

        info.dwTypeData = itemText.data();

        // I'm assuming one appends by specifying position i_insertion, the docs don't say
        const bool isPositionalArgument(true);
        if (!InsertMenuItem(menuHandle, unsigned(i_insertion), isPositionalArgument, &info))
            throw WindowsError(LOG_INFO "Failed to create menu item "s + menuEntry.text);

        ++i_insertion;
    }

    return menuHandle;
}
LOG_RETHROW

static HMENU createWindowMenu(const Menu& menu)
try
{
    HMENU const menuHandle(createMenu(menu));
    MENUINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = MIM_APPLYTOSUBMENUS | MIM_STYLE;
    info.dwStyle = MNS_NOTIFYBYPOS;
    if (!SetMenuInfo(menuHandle, &info))
        throw WindowsError(LOG_INFO "Failed to set menu info");

    return menuHandle;
}
LOG_RETHROW

void Windows::init(Config& config)
try
{
    // AddVectoredExceptionHandler reference: https://learn.microsoft.com/en-gb/windows/win32/api/errhandlingapi/nf-errhandlingapi-addvectoredexceptionhandler
    // AllocConsole reference: https://learn.microsoft.com/en-us/windows/console/allocconsole

    Os::init(config);

    // Set Windows exception handler
    if (!AddVectoredExceptionHandler(~0ul, vectoredHandler))
        throw WindowsError(LOG_INFO "Could not add vectored exception handler");

    // Create console window
    if (!AllocConsole())
        throw WindowsError(LOG_INFO "Failed to allocate console");

    if (!std::freopen("CONIN$", "r", stdin))
        throw std::runtime_error(LOG_INFO "Failed to redirect stdin");

    if (!std::freopen("CONOUT$", "w", stdout))
        throw std::runtime_error(LOG_INFO "Failed to redirect stdout");

    if (!std::freopen("CONOUT$", "w", stderr))
        throw std::runtime_error(LOG_INFO "Failed to redirect stderr");
}
LOG_RETHROW

int Windows::eventLoop()
try
{
    // Message article: https://learn.microsoft.com/en-gb/windows/win32/winmsg/about-messages-and-message-queues
    // WM_QUIT reference: https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-quit
    // GetMessage reference: https://learn.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-getmessage
    // TranslateMessage reference: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-translatemessage
    // DispatchMessage reference: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-dispatchmessage

    for (;;)
    {
        MSG msg;
        const BOOL isNotQuit(GetMessage(&msg, nullptr, 0, 0));
        
        if (isNotQuit == -1)
            throw WindowsError(LOG_INFO "Failed to get message");

        if (!isNotQuit)
            return static_cast<int>(msg.wParam);

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
LOG_RETHROW

std::filesystem::path Windows::getDataDirectory() const
try
{
    const char* const appdata(std::getenv("APPDATA"));
    if (!appdata)
        throw std::runtime_error(LOG_INFO "Could not get APPDATA environment variable");

    std::filesystem::path ret(std::filesystem::path(appdata) / "PJ"s);
    create_directories(ret);
    return ret;
}
LOG_RETHROW

static void error(const std::wstring& errorText)
try
{
    // MessageBox reference: https://learn.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-messagebox
    if (!MessageBox(nullptr, std::data(errorText), nullptr, MB_ICONERROR))
        throw WindowsError(LOG_INFO "Failed to error message box"s);
}
LOG_RETHROW

void Windows::error(const std::string& errorText) const
try
{
    ::error(toWstring(errorText));
}
LOG_RETHROW

void Windows::spawnMainWindow(MainWindow& window, std::string_view className, std::string_view title, std::any arg)
try
{
    const std::wstring className_wide(toWstring(className));
    registerClass(instance, className_wide.c_str());
    
    const std::wstring title_wide(toWstring(title));
    const int cmdShow(std::any_cast<MainWindowArg_t>(std::move(arg)));
    HMENU const menu(createWindowMenu(*window.menu));
    HWND const windowHandle(createWindow(instance, className_wide.c_str(), title_wide.c_str(), cmdShow, menu));
    windowMap[windowHandle] = &window;
}
LOG_RETHROW

void Windows::quit()
{
    // PostQuitMessage reference: https://learn.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-postquitmessage
    PostQuitMessage(EXIT_SUCCESS);
}

static std::uintptr_t CALLBACK openFileProcedure(HWND windowHandle, unsigned message, std::uintptr_t, LONG_PTR lParam) noexcept
try
{
    // Open file name hook procedure reference: https://learn.microsoft.com/en-gb/windows/win32/api/commdlg/nc-commdlg-lpofnhookproc
    // Open file name hook reference: https://learn.microsoft.com/en-gb/windows/win32/dlgbox/open-and-save-as-dialog-boxes#explorer-style-hook-procedures

    switch (message)
    {
    default:
        return false;

    // WM_INITDIALOG reference: https://learn.microsoft.com/en-gb/windows/win32/dlgbox/wm-initdialog
    case WM_INITDIALOG:
        break;

    // WM_NOTIFY reference: https://learn.microsoft.com/en-gb/windows/win32/controls/wm-notify
    case WM_NOTIFY:
    {
        // NMHDR reference: https://learn.microsoft.com/en-gb/windows/win32/api/richedit/ns-richedit-nmhdr
        // OFNOTIFY reference: https://learn.microsoft.com/en-gb/windows/win32/api/commdlg/ns-commdlg-ofnotifyw
        // CDN_FILEOK reference: https://learn.microsoft.com/en-gb/windows/win32/dlgbox/cdn-fileok
        // SetWindowLongPtr: https://learn.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-setwindowlongptrw

        const NMHDR* const p_header = reinterpret_cast<const NMHDR* const>(lParam);
        if (p_header->code != CDN_FILEOK)
            return false;

        const OFNOTIFY* const p_notification = reinterpret_cast<const OFNOTIFY* const>(p_header);
        auto& validator = *reinterpret_cast<FunctionRef<bool(std::filesystem::path)>*>(p_notification->lpOFN->lCustData);
        std::filesystem::path filepath(p_notification->lpOFN->lpstrFile);
        if (validator(std::move(filepath)))
            return false;

        SetLastError(0);
        if (!SetWindowLongPtr(windowHandle, DWLP_MSGRESULT, ~0) && GetLastError())
            throw WindowsError(LOG_INFO "Failed to reject file after failing ROM verification"s);

        error(L"Not an acceptable file");

        break;
    }
    }

    return true;
}
catch (const std::exception& e)
{
    DebugFile(DebugFile::error) << LOG_INFO << e.what() << '\n';
    return false;
}

std::optional<std::filesystem::path> Windows::chooseFile(std::span<const FileFilter> fileFilters, FunctionRef<bool(const std::filesystem::path&)> validator) const
try
{
    // GetOpenFileName reference: https://learn.microsoft.com/en-gb/windows/win32/api/commdlg/nf-commdlg-getopenfilenamew
    // CommDlgExtendedError reference: https://learn.microsoft.com/en-gb/windows/win32/api/commdlg/nf-commdlg-commdlgextendederror
    // OPENFILENAME reference: https://learn.microsoft.com/en-us/windows/win32/api/commdlg/ns-commdlg-openfilenamew

    std::wstring fileFilters_os;
    for (FileFilter fileFilter : fileFilters)
    {
        fileFilters_os += toWstring(fileFilter.label);
        fileFilters_os.push_back(L'\0');
        fileFilters_os += toWstring(fileFilter.glob);
        fileFilters_os.push_back(L'\0');
    }

    fileFilters_os += L"All files\0*\0"sv;


    wchar_t filepath[0x100]; // Arbitrary. Unsure how I wanna handle larger file paths
    filepath[0] = L'\0';

    OPENFILENAME ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = fileFilters_os.c_str();
    ofn.lpstrFile = std::data(filepath);
    ofn.nMaxFile = static_cast<unsigned long>(std::size(filepath));
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING;
    ofn.lpfnHook = openFileProcedure;
    ofn.lCustData = reinterpret_cast<LONG_PTR>(&validator);
    if (!GetOpenFileName(&ofn))
    {
        unsigned long error(CommDlgExtendedError());
        if (error)
            throw CommonDialogError(error);

        // Cancelled
        return {};
    }

    return std::filesystem::path(filepath);
}
LOG_RETHROW
