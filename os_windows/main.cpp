// Windows API reference: https://msdn.microsoft.com/en-us/library/aa383749
// Windows API reference - functions in alphabetical order: https://msdn.microsoft.com/en-us/library/aa383688
// Windows API reference - functions by category: https://msdn.microsoft.com/en-us/library/aa383686
// Windows API reference - data types: https://msdn.microsoft.com/en-us/library/aa383751
// Other Windows API reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ff818516
// Window article: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632598
// Controls reference: https://docs.microsoft.com/en-us/windows/desktop/controls/window-controls
// Control class names: https://docs.microsoft.com/en-us/windows/win32/controls/common-control-window-classes
    
#include "../os_windows.h"

#include "../resource.h"
#include "../rom.h"

#include "../global.h"

#include <cairomm/win32_surface.h>

#include <commdlg.h>

#include <codecvt>
#include <ios>
#include <locale>
#include <memory>
#include <stdexcept>


std::string toString(const std::wstring& from) noexcept
{
    // TODO: wstring_convert et al are deprecated as of C++17
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(from);
}

std::wstring toWstring(const std::string& from) noexcept
{
    // TODO: wstring_convert et al are deprecated as of C++17
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(from);
}


namespace Menu
{
/*
    Extended menu format:
        Header: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647567
        Item: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647569

    I'm defining menu manually here rather than using the resource editor because this is the only way I can give submenus an identifier,
    presumably because the resource editor uses the old menu format, which doesn't support submenus having an ID(?)
*/

    struct Header
    {
        unsigned short version{1}, offset{4};
        unsigned long helpId{};
    };

    enum struct ResInfo : unsigned short
    {
        isSubmenu = 0x01,
        isLastItem = 0x80
    };
    
    constexpr ResInfo operator|(ResInfo lhs, ResInfo rhs) noexcept
    {
        return ResInfo(toInt(lhs) | toInt(rhs));
    }

    enum struct Id : unsigned long
    {
        dummy = 1,
        file,
        open,
        recent,
        exit,
        tools,
        spritemapViewer,
        help,
        about
    };

    template<size_t n_text>
    struct Item
    {
        unsigned long type{}, state{}, menuId;
        unsigned short resInfo{};
        wchar_t text[n_text]{};
        
        constexpr Item(Id menuId, const wchar_t(&text_in)[n_text], ResInfo resInfo = {}) noexcept
            : menuId(toInt(menuId)), resInfo(toInt(resInfo))
        {
            for (size_t i(0); i < n_text; ++i)
                text[i] = text_in[i];
        }
    };

    template<size_t n_text>
    struct Submenu
    {
        Item<n_text> item;
        unsigned long helpId{};
        
        constexpr Submenu(Id menuId, const wchar_t(&text)[n_text], ResInfo resInfo = {}) noexcept
            : item(menuId, text, resInfo | ResInfo::isSubmenu)
        {}
    };

    namespace Items
    {
        constexpr auto dummy{Item(Id::dummy, L"", ResInfo::isLastItem)};

        constexpr auto file{Submenu(Id::file, L"&File")};
            constexpr auto open{Item(Id::open, L"&Open")};
            constexpr auto recent{Submenu(Id::recent, L"&Recent")};
                /* Dummy */
            constexpr auto exit{Item(Id::exit, L"E&xit", ResInfo::isLastItem)};
        constexpr auto tools{Submenu(Id::tools, L"&Tools")};
            constexpr auto spritemapViewer{Item(Id::spritemapViewer, L"&Spritemap viewer", ResInfo::isLastItem)};
        constexpr auto help{Submenu(Id::help, L"&Help", ResInfo::isLastItem)};
            constexpr auto about{Item(Id::about, L"&About", ResInfo::isLastItem)};
    };

    constexpr struct
    {
    #define ITEM(name) decltype(Items::name) name{Items::name};
    #define DUMMY(name) decltype(Items::dummy) temp_##name{Items::dummy};
        Header header{};
        ITEM(file)
            ITEM(open)
            ITEM(recent)
                DUMMY(recent)
            ITEM(exit)
        ITEM(tools)
            ITEM(spritemapViewer)
        ITEM(help)
            ITEM(about)
    #undef DUMMY
    #undef ITEM
    } menu;
}


// Static / non-member functions
std::intptr_t CALLBACK aboutProcedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR) noexcept
try
{
    // Dialog procedure reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms645469
    // Dialog box reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632588

    switch (message)
    {
    default:
        return false;

    // WM_INITDIALOG reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms645428
    case WM_INITDIALOG:
        break;

    // WM_COMMAND reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647591
    case WM_COMMAND:
    {
        const WORD id = LOWORD(wParam);
        switch (id)
        {
        default:
            throw WindowsError(LOG_INFO "Unrecognised command identifier: " + std::to_string(id));

        case IDOK:
        case IDCANCEL:
        {
            // EndDialog reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms645472
            const std::intptr_t ret(id);
            if (!EndDialog(window, ret))
                throw WindowsError(LOG_INFO "Failed to close about dialog");

            break;
        }
        }

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

std::uintptr_t CALLBACK Windows::openRomHookProcedure(HWND window, unsigned message, std::uintptr_t, LONG_PTR lParam) noexcept
try
{
    // Open file name hook procedure reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646931
    // Open file name hook reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646960#_win32_Explorer_Style_Hook_Procedures

    switch (message)
    {
    default:
        return false;

    // WM_INITDIALOG reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms645428
    case WM_INITDIALOG:
        break;

    // WM_NOTIFY reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb775583
    case WM_NOTIFY:
    {
        // NMHDR reference: https://msdn.microsoft.com/en-us/library/windows/desktop/bb775514
        // OFNOTIFY reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646836
        // CDN_FILEOK reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646857
        // SetWindowLongPtr: https://msdn.microsoft.com/en-us/library/windows/desktop/ms644898

        const NMHDR* const p_header = reinterpret_cast<const NMHDR* const>(lParam);
        if (p_header->code != CDN_FILEOK)
            return false;

        const OFNOTIFY* const p_notification = reinterpret_cast<const OFNOTIFY* const>(p_header);
        if (Rom::verifyRom(p_notification->lpOFN->lpstrFile))
            return false;

        SetLastError(0);
        if (!SetWindowLongPtr(window, DWLP_MSGRESULT, ~0) && GetLastError())
            throw WindowsError(LOG_INFO "Failed to reject file after failing ROM verification"s);

        p_windows->error(L"Not a valid ROM");

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

LRESULT CALLBACK Windows::windowProcedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR lParam) noexcept
try
{
    // Window precedure reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632593

    const auto defaultHandler([&]()
    {
        return DefWindowProc(window, message, wParam, lParam);
    });

    switch (message)
    {
    default:
        return defaultHandler();

    // WM_COMMAND reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647591
    case WM_COMMAND:
    {
        const bool isControl(lParam != 0);
        const unsigned short id(LOWORD(wParam));

        if (isControl)
            return defaultHandler();
        
        const bool isAccelerator(HIWORD(wParam) != 0);
        if (!isAccelerator)
            throw WindowsError(LOG_INFO "Received menu command in WM_COMMAND message");

        p_windows->handleCommand(id, isAccelerator);

        break;
    }

    // WM_DESTROY reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632620
    case WM_DESTROY:
    {
        // PostQuitMessage reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms644945
        try
        {
            //p_windows->config.save();
        }
        catch (const std::exception& e)
        {
            DebugFile(DebugFile::error) << LOG_INFO << "Failed to save config after opening file: "s << e.what() << '\n';
        }

        PostQuitMessage(EXIT_SUCCESS);
        
        break;
    }

    // WM_INITMENUPOPUP reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646347
    case WM_INITMENUPOPUP:
    {
        // MENUITEMINFO reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647578
        // GetMenuItemInfo reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647980
        // InsertMenuItem reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647988
        // DeleteMenu reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647629
        // GetItemMenuCount reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647978

        const HMENU menu(reinterpret_cast<HMENU>(wParam));
        const unsigned short superMenuIndex(LOWORD(lParam));
        const bool isWindowMenu(HIWORD(lParam));

        if (isWindowMenu)
            return defaultHandler();

    /*
        What I would like to have done is to get the ID of the menu item and compare it to the "recent" menu's ID `Menu::menu.recent.item.menuId`.
        The menu ID is accessible via the wID member of the `MENUITEMINFO` struct, which is populated by `GetMenuItemInfo`.
        The problem is that `GetMenuItemInfo` requires a parent menu handle (and either the index of the item within that menu or the ID of that item).
        I have the index of the menu item within the parent menu, but only have the handle of the menu itself and not the parent menu, with no functions in the API that get the parent menu.

        The result being that to check if menu being initialised is the 'recent' menu, I'm comparing the handle `menu` to the handle of the 'recent' submenu,
        which fortunately can be found with its ID by using `GetMenuItemInfo` on the window menu.

        N.B. There is a function `GetMenuItemID`, which *would* work like simpler `GetMenuItemInfo`, but for some reason is defined to return `-1` for submenus...
    */
        MENUITEMINFO recentSubmenuInfo{};
        recentSubmenuInfo.cbSize = sizeof(recentSubmenuInfo);
        recentSubmenuInfo.fMask  = MIIM_SUBMENU;
        if (!GetMenuItemInfo(GetMenu(window), Menu::menu.recent.item.menuId, MF_BYCOMMAND, &recentSubmenuInfo))
            throw WindowsError(LOG_INFO "Failed to get recent submenu item info"s);

        if (menu == recentSubmenuInfo.hSubMenu)
        {
            // Populate recent files from config (clearing the submenu first)

            int n_menuItems(GetMenuItemCount(menu));
            if (n_menuItems == -1)
                throw WindowsError(LOG_INFO "Failed to get number of menu items"s);

            while (n_menuItems--)
                if (!DeleteMenu(menu, 0, MF_BYPOSITION))
                    throw WindowsError(LOG_INFO "Failed to delete menu item"s);

            MENUITEMINFO menuItemInfo{};
            menuItemInfo.cbSize = sizeof(menuItemInfo);
            menuItemInfo.fMask = MIIM_TYPE;
            for (std::wstring filepath : Windows::p_windows->config.recentFiles)
            {
                menuItemInfo.dwTypeData = std::data(filepath);
                if (!InsertMenuItem(menu, 0, MF_BYPOSITION, &menuItemInfo))
                    throw WindowsError(LOG_INFO "Failed to insert menu item"s);
            }
        }
        else
            return defaultHandler();

        break;
    }

    // WM_MENUCOMMAND reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647603
    case WM_MENUCOMMAND:
    {
        // MENUITEMINFO reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647578
        // GetMenuItemInfo reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647980

        const unsigned menuIndex(static_cast<unsigned>(wParam));
        const HMENU menu(reinterpret_cast<HMENU>(lParam));

        MENUITEMINFO recentSubmenuInfo{};
        recentSubmenuInfo.cbSize = sizeof(recentSubmenuInfo);
        recentSubmenuInfo.fMask  = MIIM_SUBMENU;
        if (!GetMenuItemInfo(GetMenu(window), Menu::menu.recent.item.menuId, MF_BYCOMMAND, &recentSubmenuInfo))
            throw WindowsError(LOG_INFO "Failed to get recent submenu item info"s);

        MENUITEMINFO menuItemInfo{};
        menuItemInfo.cbSize = sizeof(menuItemInfo);
        if (menu == recentSubmenuInfo.hSubMenu)
        {
            menuItemInfo.fMask = MIIM_STRING;
            if (!GetMenuItemInfo(menu, menuIndex, MF_BYPOSITION, &menuItemInfo))
                throw WindowsError(LOG_INFO "Failed to get menu item info"s);

            std::wstring filepath(menuItemInfo.cch + 1, L'\0');
            menuItemInfo.dwTypeData = std::data(filepath);
            ++menuItemInfo.cch;
            if (!GetMenuItemInfo(menu, menuIndex, MF_BYPOSITION, &menuItemInfo))
                throw WindowsError(LOG_INFO "Failed to get menu item info"s);

            p_windows->openRom(filepath);

            break;
        }

        menuItemInfo.fMask = MIIM_ID;
        if (!GetMenuItemInfo(menu, menuIndex, MF_BYPOSITION, &menuItemInfo))
            throw WindowsError(LOG_INFO "Failed to get menu item info"s);

        p_windows->handleCommand(menuItemInfo.wID, false);

        break;
    }

    // WM_NOTIFY reference: https://docs.microsoft.com/en-us/windows/desktop/controls/wm-notify
    case WM_NOTIFY:
    {
        // NMHDR reference: https://docs.microsoft.com/en-us/windows/desktop/api/richedit/ns-richedit-_nmhdr

        const NMHDR* const p_nmhdr(reinterpret_cast<NMHDR*>(lParam));
        switch (p_nmhdr->code)
        {
        default:
            return defaultHandler();

        // TVN_SELCHANGED reference: https://docs.microsoft.com/en-us/windows/desktop/controls/tvn-selchanged
        case TVN_SELCHANGED:
        {
            // NMTREEVIEW reference: https://docs.microsoft.com/en-us/windows/desktop/api/Commctrl/ns-commctrl-tagnmtreeviewa
            // TreeView_GetParent reference: https://docs.microsoft.com/en-us/windows/desktop/api/Commctrl/nf-commctrl-treeview_getparent
            // TreeView_GetItem reference: https://docs.microsoft.com/en-us/windows/desktop/api/commctrl/nf-commctrl-treeview_getitem
            // TVITEMEX reference: https://docs.microsoft.com/en-us/windows/desktop/api/Commctrl/ns-commctrl-tagtvitemexa
            // InvalidateRect reference: https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-invalidaterect

            const NMTREEVIEW* const p_treeview(reinterpret_cast<const NMTREEVIEW*>(p_nmhdr));
            if (p_nmhdr->hwndFrom != p_windows->p_roomSelectorTree->window)
                throw std::runtime_error(LOG_INFO "Unknown tree view handle"s);

            TVITEMEX item{};
            item.mask = TVIF_CHILDREN | TVIF_HANDLE;
            item.hItem = p_treeview->itemNew.hItem;
            if (!TreeView_GetItem(p_windows->p_roomSelectorTree->window, &item))
                throw WindowsError(LOG_INFO "Failed to get tree view item"s);

            // Only processing leaf nodes
            if (item.cChildren != 0)
                return defaultHandler();

            std::vector<long> ids;
            for (HTREEITEM hItem(p_treeview->itemNew.hItem); hItem; hItem = TreeView_GetParent(p_windows->p_roomSelectorTree->window, hItem))
            {
                TVITEMEX item{};
                item.mask = TVIF_PARAM | TVIF_HANDLE;
                item.hItem = hItem;
                if (!TreeView_GetItem(p_windows->p_roomSelectorTree->window, &item))
                    throw WindowsError(LOG_INFO "Failed to get tree view item"s);

                ids.push_back(long(item.lParam));
            }

            std::reverse(std::begin(ids), std::end(ids));
            p_windows->p_rom->loadLevelData(std::move(ids));
            p_windows->updateLevelViewScrollbarDimensions();
            if (!InvalidateRect(p_windows->p_levelView->window, nullptr, true))
                throw WindowsError(LOG_INFO "Failed to invalidate level view window"s);

            break;
        }
        }
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

    // WM_SIZE reference: https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-size
    case WM_SIZE:
    {
        if (!p_windows->p_windowLayout)
            return defaultHandler();

        if (wParam != SIZE_RESTORED && wParam != SIZE_MAXIMIZED)
            return defaultHandler();

        const int width(LOWORD(lParam)), height(HIWORD(lParam));
        p_windows->p_windowLayout->resize(0, 0, width, height);
        p_windows->updateLevelViewScrollbarDimensions();
        
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

long CALLBACK vectoredHandler(EXCEPTION_POINTERS* p_e) noexcept
{
    // VectoredHandler reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms681419

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


// Member functions
Windows::Windows(HINSTANCE instance, int cmdShow, Config& config) noexcept
    : instance(instance), cmdShow(cmdShow), config(config)
{
    // No logging allowed here

    // SetErrorMode reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms680621

    p_windows = this;

    // Do not display the Windows error reporting dialog
    SetErrorMode(SEM_NOGPFAULTERRORBOX);
}

void Windows::init()
try
{
    // AddVectoredExceptionHandler reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms679274
    // LoadAccelerators reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646370
    // CreateFont reference: https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createfonta

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

    // Create main window
    registerClass();
    createWindow();

    // Load keyboard shortcuts
    accelerators = LoadAccelerators(instance, MAKEINTRESOURCE(IDC_METROIDLEVELEDITOR));
    if (!accelerators)
        throw WindowsError(LOG_INFO "Failed to load acclerators");

    // Load monospace font
    monospace = CreateFont(16, 0, 0, 0, FW_DONTCARE, false, false, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    if (!monospace )
        throw WindowsError(LOG_INFO "Could not load monospace font"s);
}
LOG_RETHROW

void Windows::registerClass()
try
{
    // Window class article: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633574
    // WNDCLASSEXW reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633577
    // Window class style constants: https://msdn.microsoft.com/en-us/library/windows/desktop/ff729176
    // LoadImage reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms648045
    // CreateSolidBrush reference: https://msdn.microsoft.com/en-us/library/dd183518
    // RegisterClassEx reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633587

    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW; // Redraw the entire window if a movement or size adjustment changes the width or height of the client area
    wcex.lpfnWndProc = windowProcedure;
    wcex.hInstance = instance;
    wcex.hIcon = static_cast<HICON>(LoadImage(instance, MAKEINTRESOURCE(IDI_METROIDLEVELEDITOR), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
    if (!wcex.hIcon)
        throw WindowsError(LOG_INFO "Failed to load icon");

    wcex.hCursor = static_cast<HCURSOR>(LoadImage(nullptr, MAKEINTRESOURCE(OCR_NORMAL), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    if (!wcex.hCursor)
        throw WindowsError(LOG_INFO "Failed to load cursor");

    wcex.hbrBackground = CreateSolidBrush(0x000000);
    if (!wcex.hbrBackground)
        throw WindowsError(LOG_INFO "Failed to create background brush");

    wcex.lpszClassName = className;
    wcex.hIconSm = static_cast<HICON>(LoadImage(instance, MAKEINTRESOURCE(IDI_SMALL), IMAGE_ICON, SM_CXSMICON, SM_CYSMICON, LR_DEFAULTCOLOR));
    if (!wcex.hIconSm)
        throw WindowsError(LOG_INFO "Failed to load small icon");

    ATOM registeredClass(RegisterClassEx(&wcex));
    if (!registeredClass)
        throw WindowsError(LOG_INFO "Failed to register main window class");
}
LOG_RETHROW

void Windows::createWindow()
try
{
    // CreateWindowEx reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632680
    // Window style constants: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632600
    // Window extended style constants: https://msdn.microsoft.com/en-us/library/windows/desktop/ff700543
    // LoadMenuIndirect reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647991

    // style & WS_VISIBLE shows the window after creation.
    // When x == CW_USEDEFAULT and y != CW_USEDEFAULT, y is used as the cmdShow parameter
    const unsigned long exStyle{};
    const unsigned long style(WS_TILEDWINDOW | WS_VISIBLE);
    const int x(CW_USEDEFAULT);
    const int y(cmdShow);
    const int width(CW_USEDEFAULT);
    const int height(CW_USEDEFAULT);
    HWND const windowParent{};
    HMENU const menu(LoadMenuIndirect(&Menu::menu));
    if (!menu)
        throw WindowsError(LOG_INFO "Failed to load menu");

    MENUINFO menuInfo{};
    menuInfo.cbSize = sizeof(menuInfo);
    menuInfo.fMask = MIM_STYLE;
    menuInfo.dwStyle = MNS_NOTIFYBYPOS;
    if (!SetMenuInfo(menu, &menuInfo))
        throw WindowsError(LOG_INFO "Failed to set menu info");

    void* const param{};
    window = CreateWindowEx(exStyle, className, titleString, style, x, y, width, height, windowParent, menu, instance, param);
    if (!window)
        throw WindowsError(LOG_INFO "Failed to create main window");
}
LOG_RETHROW

int Windows::eventLoop()
try
{
    // Message article: https://msdn.microsoft.com/en-us/library/windows/desktop/ms644927
    // WM_QUIT reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms644945
    // Accelerator reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms645526
    // GetMessage reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms644936
    // TranslateAccelerator reference: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-translateacceleratora
    // TranslateMessage reference: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-translatemessage
    // DispatchMessage reference: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-dispatchmessage
    // IsDialogMessage reference: https://docs.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-isdialogmessagea

    for (;;)
    {
        MSG msg;
        const BOOL isNotQuit(GetMessage(&msg, nullptr, 0, 0));
        
        if (isNotQuit == -1)
            throw WindowsError(LOG_INFO "Failed to get message");

        if (!isNotQuit)
            return static_cast<int>(msg.wParam);

        // Intercept keyboard interface messages (in particular, tabs), which are sent to the spritemap controls (address inputs), so that they can be processed by the window (otherwise nothing happens)
        // Note that this prevents arrow keys from working outside of navigating the controls for the spritemap viewer window (such as scrolling)
        if (p_spritemapViewer && p_spritemapViewer->window && IsDialogMessage(p_spritemapViewer->window, &msg))
            continue;
        
        if (TranslateAccelerator(msg.hwnd, accelerators, &msg))
            continue;

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

    const auto ret(std::filesystem::path(appdata) / "PJ"s);
    create_directories(ret);
    return ret;
}
LOG_RETHROW

void Windows::error(const std::wstring& errorText) const
try
{
    // MessageBox reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms645505
    if (!MessageBox(nullptr, std::data(errorText), nullptr, MB_ICONERROR))
        throw WindowsError(LOG_INFO "Failed to error message box"s);
}
LOG_RETHROW

void Windows::error(const std::string& errorText) const
try
{
    error(toWstring(errorText));
}
LOG_RETHROW

void Windows::createChildWindows()
try
{
    // RECT reference: https://msdn.microsoft.com/en-us/library/windows/desktop/dd162897
    // INITCOMMONCONTROLSEX reference: https://docs.microsoft.com/en-gb/windows/desktop/api/commctrl/ns-commctrl-taginitcommoncontrolsex
    // GetClientRec reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633503
    // InitCommonControlsEx reference: https://docs.microsoft.com/en-gb/windows/desktop/api/commctrl/nf-commctrl-initcommoncontrolsex
    
    INITCOMMONCONTROLSEX iccs{};
    iccs.dwSize = sizeof(iccs);
    iccs.dwICC = ICC_TREEVIEW_CLASSES;
    if (!InitCommonControlsEx(&iccs))
        throw std::runtime_error(LOG_INFO "Could not initialise common controls"s);

    RECT rect;
    if (!GetClientRect(window, &rect))
        throw WindowsError(LOG_INFO "Failed to get size of client area of main window"s);

    if (!p_levelView)
        p_levelView = std::make_unique<LevelView>(*this);

    if (!p_roomSelectorTree)
        p_roomSelectorTree = std::make_unique<RoomSelectorTree>(*this);

    p_windowLayout = WindowRow::make
    ({
        {2./3, p_levelView.get()},
        {1./3, p_roomSelectorTree.get()}
    });

    p_windowLayout->create(0, 0, rect.right, rect.bottom, window);
}
LOG_RETHROW

void Windows::destroyChildWindows()
try
{
    if (p_windows->p_levelView)
        p_windows->p_levelView->destroy();

    if (p_windows->p_roomSelectorTree)
        p_windows->p_roomSelectorTree->destroy();
}
LOG_RETHROW

void Windows::handleCommand(unsigned id, bool isAccelerator)
try
{
    switch (id)
    {
        default:
            throw WindowsError(LOG_INFO "Unrecognised "s + (isAccelerator ? "accelerator"s : "menu"s) + " command identifier: "s + std::to_string(id));

        case Menu::menu.open.menuId:
        {
            // GetOpenFileName reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646927
            // CommDlgExtendedError: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646916

            wchar_t filepath[0x100];
            filepath[0] = L'\0';

            OPENFILENAME ofn{};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = window;
            ofn.lpstrFilter =
                L"ROM files\0*.abg;*.gba;*.smc;*.sfc;\0"
                L"GBA ROM files\0*.abg;*.gba;\0"
                L"SNES ROM files\0*.smc;*.sfc;\0"
                L"All files\0*\0";

            ofn.lpstrFile = std::data(filepath);
            ofn.nMaxFile = static_cast<unsigned long>(std::size(filepath));
            /*
            // Modern dialog
            ofn.Flags = OFN_FILEMUSTEXIST;
            /*/
            // Classic dialog, but verifies ROMs
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_ENABLEHOOK | OFN_EXPLORER | OFN_ENABLESIZING;
            ofn.lpfnHook = openRomHookProcedure;
            //*/
            if (!GetOpenFileName(&ofn))
            {
                unsigned long error(CommDlgExtendedError());
                if (error)
                    throw CommonDialogError(error);

                // Cancelled
                break;
            }

            openRom(filepath);

            break;
        }

        case Menu::menu.spritemapViewer.menuId:
        {
            if (!p_spritemapViewer)
                p_spritemapViewer = std::make_unique<SpritemapViewer>(*this); // Constructor registers the class if it has a window procedure
            else
                p_spritemapViewer->destroy();

            // if window has been destroyed by e.g. close button, destroy fails because of call on invalid window handle;
            // I'm destroying the window here because I haven't made support for multiple spritemap viewers open,
            // which would require a different method of accessing member variables from the window procedure (than the static instance pointer)...
            // probably a map from window handle to instance pointer.

            // I could add a hook on the destroy notification to clear the window handle, but the result would be messy w.r.t. inheritence;
            // would rather remove this call to destroy instead.
            // Then again, the destroy method should be able to handle destruction at any point, changing ROM will destroy the windows and can happen at any time.
            
            // Maybe use a windows API function to check if the handle is valid? Not my preferred option...
            // Maybe there is a tidy way of adding a window destruction hook?
            // Possible approach: make base window precedure that looks up message in a map from message to handler, initialised with a destruction handler, and derived classes have to add to the map.
            // Has a small runtime overhead compared to a switch-case, but it would work

            p_spritemapViewer->create();

            break;
        }

        case Menu::menu.about.menuId:
        {
            // DialogBox reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms645452
            // Can use CreateDialog for a modeless version
            const std::intptr_t dialogRet(DialogBox(nullptr, MAKEINTRESOURCE(IDD_ABOUTBOX), window, aboutProcedure));
            if (dialogRet == 0 || dialogRet == -1)
                throw WindowsError(LOG_INFO "Failed to open about dialog box");

            break;
        }

        case Menu::menu.exit.menuId:
        {
            // DestroyWindow reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632682
            if (!DestroyWindow(window))
                throw WindowsError(LOG_INFO "Failed to destroy window on exit");

            break;
        }
    }
}
LOG_RETHROW

void Windows::openRom(std::filesystem::path filepath)
try
{
    try
    {
        p_rom = Rom::loadRom(filepath);
    }
    catch (const std::exception& e)
    {
        error(e.what());
        throw;
    }

    try
    {
        config.addRecentFile(filepath);
        config.save();
    }
    catch (const std::exception& e)
    {
        DebugFile(DebugFile::error) << LOG_INFO "Failed to add and save file to config: "s << e.what() << '\n';
    }

    // Load room data / whatever other tool data
    // Create child windows
    // Level view, tile table, plaintext notes, room/area selector, other tools that change the level view (tileset selector, room fx)
    createChildWindows();
}
LOG_RETHROW

void Windows::updateLevelViewScrollbarDimensions()
try
{
    // SCROLLINFO reference: https://docs.microsoft.com/en-gb/windows/desktop/api/winuser/ns-winuser-tagscrollinfo
    // SetScrollInfo reference: https://docs.microsoft.com/en-us/windows/desktop/api/Winuser/nf-winuser-setscrollinfo
    // GetClientRect reference: https://docs.microsoft.com/en-us/windows/desktop/api/winuser/nf-winuser-getclientrect

    if (!p_rom)
        return;

    const Rom::Dimensions dimensions(p_rom->getLevelViewDimensions());
    RECT clientRect;
    if (!GetClientRect(p_levelView->window, &clientRect))
        throw WindowsError(LOG_INFO "Could not get client rect"s);

    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE | SIF_RANGE;
    si.nMin = 0;
    
    si.nPage = clientRect.bottom / dimensions.blockSize;
    si.nMax = int(dimensions.n_y - 1);
    SetScrollInfo(p_levelView->window, SB_VERT, &si, true);

    si.nPage = clientRect.right / dimensions.blockSize;
    si.nMax = int(dimensions.n_x - 1);
    SetScrollInfo(p_levelView->window, SB_HORZ, &si, true);
}
LOG_RETHROW


// RoomSelectorTree
void Windows::RoomSelectorTree::create(int x, int y, int width, int height, HWND hwnd)
try
{
    Window::create(x, y, width, height, hwnd);
    insertRoomList(windows.p_rom->getRoomList());
}
LOG_RETHROW

void Windows::RoomSelectorTree::insertRoomList(const std::vector<Rom::RoomList>& roomLists, HTREEITEM parent /*= TVI_ROOT*/)
try
{
    // TreeView_InsertItem reference: https://docs.microsoft.com/en-us/windows/desktop/api/Commctrl/nf-commctrl-treeview_insertitem
    // TV_INSERTSTRUCT reference: https://docs.microsoft.com/en-gb/windows/desktop/api/commctrl/ns-commctrl-tagtvinsertstructa

    TV_INSERTSTRUCT is{};
    is.hInsertAfter = TVI_LAST;
    is.itemex.mask = TVIF_TEXT | TVIF_PARAM;
    for (const Rom::RoomList& roomList : roomLists)
    {
        is.hParent = parent;
        std::wstring text(toWstring(roomList.name));
        is.itemex.pszText = std::data(text);
        is.itemex.lParam = roomList.id;
        HTREEITEM item(TreeView_InsertItem(window, &is));
        if (!item)
            throw WindowsError(LOG_INFO "Failed to insert item into room selector tree"s);

        insertRoomList(roomList.subrooms, item);
    }
}
LOG_RETHROW
