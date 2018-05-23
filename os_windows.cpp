// Windows API reference: https://msdn.microsoft.com/en-us/library/aa383749
// Windows API reference - functions in alphabetical order: https://msdn.microsoft.com/en-us/library/aa383688
// Windows API reference - functions by category: https://msdn.microsoft.com/en-us/library/aa383686
// Windows API reference - data types: https://msdn.microsoft.com/en-us/library/aa383751
// Other Windows API reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ff818516
// Window article: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632598

#include "os_windows.h"
#include "resource.h"

#include "global.h"
    
#include <commdlg.h>

#include <codecvt>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
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

class WindowsError : public std::runtime_error
{
    // GetLastError reference: https://msdn.microsoft.com/en-gb/library/windows/desktop/ms679360

protected:
    // FormatMessage reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms679351
    static std::wstring getErrorMessage(unsigned long errorId)
    try
    {
        const unsigned long flags(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS);
        const void* const source{};
        const unsigned long languageId(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
        wchar_t* formattedMessage;
        const unsigned long size{};
        if (!FormatMessage(flags, source, errorId, languageId, reinterpret_cast<wchar_t*>(&formattedMessage), size, nullptr))
            return L"Unknown error. FormatMessage failed with error code " + std::to_wstring(GetLastError());
        
        std::wstring ret(formattedMessage);
        LocalFree(formattedMessage);
        return ret;
    }
    LOG_RETHROW

    // TODO: add stack trace
    // https://msdn.microsoft.com/en-us/library/windows/desktop/bb204633
    static std::string makeMessage() noexcept
    {
        return makeMessage(GetLastError());
    }

    static std::string makeMessage(const std::string& extraMessage) noexcept
    {
        return makeMessage(GetLastError(), extraMessage);
    }

    static std::string makeMessage(unsigned long errorId) noexcept
    try
    {
        const std::string errorMessage(toString(getErrorMessage(errorId)));
        return "Win32 API error occurred, error code " + std::to_string(errorId) + ": " + errorMessage;
    }
    catch (...)
    {
        return "Win32 API error occurred. Unknown error. An exception was thrown during error message construction.";
    }

    static std::string makeMessage(unsigned long errorId, const std::string& extraMessage) noexcept
    try
    {
        const std::string errorMessage(toString(getErrorMessage(errorId)));
        return makeMessage(errorId) + ' ' + extraMessage;
    }
    catch (...)
    {
        return "Win32 API error occurred. Unknown error. An exception was thrown during error message construction.";
    }

public:
    WindowsError() noexcept
        : std::runtime_error(makeMessage())
    {}

    WindowsError(unsigned long errorId) noexcept
        : std::runtime_error(makeMessage(errorId))
    {}

    WindowsError(const std::string& extraMessage) noexcept
        : std::runtime_error(makeMessage(extraMessage))
    {}

    WindowsError(unsigned long errorId, const std::string& extraMessage) noexcept
        : std::runtime_error(makeMessage(errorId, extraMessage))
    {}
};

namespace Menu
{
/*
    Extended menu format:
        Header: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647567
        Item: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647569
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
        ITEM(help)
            ITEM(about)
    } menu;
}

std::intptr_t CALLBACK aboutPrecedure(HWND window, unsigned message, std::uintptr_t wParam, std::intptr_t) noexcept
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
            throw WindowsError("Unrecognised command identifier: " + std::to_string(id));

        case IDOK:
        case IDCANCEL:
        {
            // EndDialog reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms645472
            const intptr_t ret(id);
            if (!EndDialog(window, ret))
                throw WindowsError("Failed to close about dialog");

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
    return true;
}

std::intptr_t CALLBACK windowPrecedure(HWND window, unsigned message, std::uintptr_t wParam, std::intptr_t lParam) noexcept
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
        const WORD id(LOWORD(wParam));

        if (isControl)
            return defaultHandler();
        
        const bool isAccelerator(HIWORD(wParam) != 0);
        switch (id)
        {
        default:
            throw WindowsError("Unrecognised "s + (isAccelerator ? "accelerator"s : "menu"s) + " command identifier: "s + std::to_string(id));

        case Menu::menu.open.menuId:
        {
            // GetOpenFileName reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646927
            // CommDlgExtendedError: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646916

            // Grabbed this from old code. TODO: docs say this is supersceded by 'Common Item Dialog'

            // Note that the initial directory is given the first time by filename or initialDirectory (in that order),
            // successive times are given by the previously chosen directory if initialDirectory hasn't changed (otherwise the above);
            // otherwise, the current directory is used if it has any files matching the default filter, otherwise the user' home folder or the desktop

            // TODO: use OFN_ENABLEHOOK | OFN_ENABLEINCLUDENOTIFY with a hook function to list only those files whose ROM header is supported
            // Note that if a hooking function is specified, OFN_ENABLESIZING | OFN_EXPLORER should also be specified

            wchar_t filepath[0x100];
            filepath[0] = L'\0';

            const DWORD structSize(sizeof(OPENFILENAME));
            const HWND owner(window);
            const HINSTANCE instance{};
            const wchar_t* const filter
            (
                L"GBA ROM files\0*.abg;*.gba;\0"
                L"All files\0*\0"
            );

            // Optional buffer for user-writable filter
            wchar_t* const customFilter{};
            const DWORD n_customFilter{};

            // Optionally specify the index of the default filter
            const DWORD filterIndex{};

            wchar_t* const p_filepath(filepath);
            DWORD n_filepath(DWORD(std::size(filepath)));

            // Optional buffer for a copy of the filename.extension component
            wchar_t* const filename{};
            const DWORD n_filename{};

            // Optional initial directory
            wchar_t* const initialDirectory{};

            // Optional title of window (default: "Open")
            wchar_t* const title{};

            const DWORD flags(OFN_FILEMUSTEXIST);

            // Will contain the index of the first character of the filename within filepath
            const WORD i_filename{};

            // Will contain the index of the first character of the file extension within filepath or 0 if there is no extension
            const WORD i_fileExtension{};

            // Optional extension to add to filepath if it doesn't already have one (three characters max)
            const wchar_t* const defaultExtension{};

            // Arbitrary custom data for the hook function, which receives the OPENFILENAME structure via WM_INITDIALOG
            const LPARAM customData{};

            // Optional pointer to hook function, requires OFN_ENABLEHOOK
            const LPOFNHOOKPROC hook{};

            // For loading a resource dialog, requires OFN_ENABLETEMPLATE
            wchar_t* const templateName(nullptr);

            void* const reserved_0{};
            const DWORD reserved_1{};

            // TODO: set to OFN_EX_NOPLACESBAR to hide the places sidebar based on a configuration file
            const DWORD flagsEx{};

            OPENFILENAME ofn
            {
                structSize,
                owner,
                instance,
                filter,
                customFilter,
                n_customFilter,
                filterIndex,
                p_filepath,
                n_filepath,
                filename,
                n_filename,
                initialDirectory,
                title,
                flags,
                i_filename,
                i_fileExtension,
                defaultExtension,
                customData,
                hook,
                templateName,
                reserved_0,
                reserved_1,
                flagsEx
            };
            if (!GetOpenFileName(&ofn))
            {
                const DWORD error(CommDlgExtendedError());
                if (error)
                    throw WindowsError(error);

                // Cancelled
                break;
            }

            try
            {
                Windows::p_windows->config.addRecentFile(filepath);
                Windows::p_windows->config.save();
            }
            catch (const std::exception& e)
            {
                DebugFile(DebugFile::error) << LOG_INFO "Failed to add and save file to config: "s << e.what() << '\n';
            }
            break;
        }

        case Menu::menu.about.menuId:
        {
            // DialogBox reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms645452
            // Can use CreateDialog for a modeless version
            const std::intptr_t dialogRet(DialogBox(nullptr, MAKEINTRESOURCE(IDD_ABOUTBOX), window, aboutPrecedure));
            if (dialogRet == 0 || dialogRet == -1)
                throw WindowsError("Failed to open about dialog box");

            break;
        }

        case Menu::menu.exit.menuId:
        {
            // DestroyWindow reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632682
            if (!DestroyWindow(window))
                throw WindowsError("Failed to destroy window on exit");

            break;
        }
        }

        break;
    }

    // WM_DESTROY reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632620
    case WM_DESTROY:
    {
        // PostQuitMessage reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms644945
        try
        {
            //Windows::p_windows->config.save();
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
        const unsigned superMenuIndex(LOWORD(lParam));
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

    // WM_PAINT reference: https://msdn.microsoft.com/en-us/library/dd145213
    case WM_PAINT:
    {
        // GetUpdateRect reference: https://msdn.microsoft.com/en-us/library/dd144943
        RECT updateRect;
        BOOL&& notEmpty(GetUpdateRect(window, &updateRect, false));
        if (notEmpty)
        {
            // BeginPaint reference: https://msdn.microsoft.com/en-us/library/dd183362
            // EndPaint reference: https://msdn.microsoft.com/en-us/library/dd162598
            PAINTSTRUCT ps;
            const auto endPaint([&](HDC)
            {
                EndPaint(window, &ps);
            });
            const std::unique_ptr<std::remove_pointer_t<HDC>, decltype(endPaint)> displayContext(BeginPaint(window, &ps), endPaint);
            if (!displayContext)
                throw WindowsError("Failed to get display device context from BeginPaint");

            const std::wstring displayText(L"Hello, Windows!");
            if (!TextOut(displayContext.get(), 0, 0, std::data(displayText), static_cast<int>(std::size(displayText))))
                throw WindowsError("Failed to display text");
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

    // Set Windows exception handler
    if (!AddVectoredExceptionHandler(~0ul, vectoredHandler))
        throw WindowsError("Could not add vectored exception handler");

    // Create console window
    if (!AllocConsole())
        throw WindowsError("Failed to allocate console");

    if (!std::freopen("CONIN$", "r", stdin))
        throw std::runtime_error("Failed to redirect stdin");

    if (!std::freopen("CONOUT$", "w", stdout))
        throw std::runtime_error("Failed to redirect stdout");

    if (!std::freopen("CONOUT$", "w", stderr))
        throw std::runtime_error("Failed to redirect stderr");


    // Create main window
    registerClass();
    createWindow();

    // Load keyboard shortcuts
    accelerators = LoadAccelerators(instance, MAKEINTRESOURCE(IDC_FUSIONLEVELEDITOR));
    if (!accelerators)
        throw WindowsError("Failed to load acclerators");
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
    wcex.lpfnWndProc = windowPrecedure;
    wcex.hInstance = instance;
    wcex.hIcon = static_cast<HICON>(LoadImage(instance, MAKEINTRESOURCE(IDI_FUSIONLEVELEDITOR), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
    if (!wcex.hIcon)
        throw WindowsError("Failed to load icon");

    wcex.hCursor = static_cast<HCURSOR>(LoadImage(nullptr, MAKEINTRESOURCE(OCR_NORMAL), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
    if (!wcex.hCursor)
        throw WindowsError("Failed to load cursor");

    wcex.hbrBackground = CreateSolidBrush(0x000000);
    if (!wcex.hCursor)
        throw WindowsError("Failed to create background brush");

    wcex.lpszClassName = className;
    wcex.hIconSm = static_cast<HICON>(LoadImage(instance, MAKEINTRESOURCE(IDI_SMALL), IMAGE_ICON, SM_CXSMICON, SM_CYSMICON, LR_DEFAULTCOLOR));
    if (!wcex.hIconSm)
        throw WindowsError("Failed to load small icon");

    ATOM&& registeredClass(RegisterClassEx(&wcex));
    if (!registeredClass)
        throw WindowsError("Failed to register main window class");
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
        throw WindowsError("Failed to load menu");

    void* const param{};
    HWND window = CreateWindowEx(exStyle, className, titleString, style, x, y, width, height, windowParent, menu, instance, param);
    if (!window)
        throw WindowsError("Failed to create main window");
}
LOG_RETHROW

int Windows::eventLoop()
try
{
    // Message article: https://msdn.microsoft.com/en-us/library/windows/desktop/ms644927
    // GetMessage reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms644936
    // WM_QUIT reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms644945
    // Accelerator reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms645526

    for (;;)
    {
        MSG msg;
        const BOOL isNotQuit(GetMessage(&msg, nullptr, 0, 0));
        if (isNotQuit == -1)
            throw WindowsError("Failed to get message");
        if (!isNotQuit)
            return static_cast<int>(msg.wParam);

        if (!TranslateAccelerator(msg.hwnd, accelerators, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
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
