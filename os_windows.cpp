// Windows API reference: https://msdn.microsoft.com/en-us/library/aa383749
// Windows API reference - functions in alphabetical order: https://msdn.microsoft.com/en-us/library/aa383688
// Windows API reference - functions by category: https://msdn.microsoft.com/en-us/library/aa383686
// Windows API reference - data types: https://msdn.microsoft.com/en-us/library/aa383751
// Other Windows API reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ff818516
// Window article: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632598

#include "os_windows.h"
#include "resource.h"
    
#include <commdlg.h>

#include <codecvt>
#include <cstdint>
#include <locale>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

std::string toString(const std::wstring& from)
{
    // TODO: wstring_convert et al are deprecated as of C++17
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(from);
}

class WindowsError : public std::runtime_error
{
    // GetLastError reference: https://msdn.microsoft.com/en-gb/library/windows/desktop/ms679360

protected:
    // TODO: can I call FormatMessaageA to avoid the conversion to ASCII?
    // FormatMessage reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms679351
    static std::wstring getErrorMessage(DWORD errorId)
    {
        const DWORD flags(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS);
        const void* const source{};
        const DWORD languageId(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT));
        wchar_t* formattedMessage;
        const DWORD size{};
        if (!FormatMessage(flags, source, errorId, languageId, reinterpret_cast<wchar_t*>(&formattedMessage), size, nullptr))
            return L"Unknown error. FormatMessage failed with error code " + std::to_wstring(GetLastError());
        
        std::wstring ret(formattedMessage);
        LocalFree(formattedMessage);
        return ret;
    }

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

    static std::string makeMessage(DWORD errorId) noexcept
    try
    {
        const std::string errorMessage(toString(getErrorMessage(errorId)));
        return "Win32 API error occurred, error code " + std::to_string(errorId) + ": " + errorMessage;
    }
    catch (...)
    {
        return "Unknown error. An exception was thrown during error message construction.";
    }

    static std::string makeMessage(DWORD errorId, const std::string& extraMessage) noexcept
    try
    {
        const std::string errorMessage(toString(getErrorMessage(errorId)));
        return makeMessage(errorId) + ". " + extraMessage;
    }
    catch (...)
    {
        return "Unknown error. An exception was thrown during error message construction.";
    }

public:
    WindowsError() noexcept
        : std::runtime_error(makeMessage())
    {}

    WindowsError(DWORD errorId) noexcept
        : std::runtime_error(makeMessage(errorId))
    {}

    WindowsError(const std::string& extraMessage) noexcept
        : std::runtime_error(makeMessage(extraMessage))
    {}

    WindowsError(DWORD errorId, const std::string& extraMessage) noexcept
        : std::runtime_error(makeMessage(errorId, extraMessage))
    {}
};

Windows::Windows(HINSTANCE instance, int cmdShow)
    : instance(instance)
{
    // Do not display the Windows Error Reporting dialog
    SetErrorMode(SEM_NOGPFAULTERRORBOX);

    registerClass();
    createWindow(cmdShow);
    accelerators = LoadAccelerators(instance, MAKEINTRESOURCE(IDC_FUSIONLEVELEDITOR));
    if (!accelerators)
        throw WindowsError("Failed to load acclerators");
}

void Windows::registerClass()
{
    // Window class article: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633574
    // WNDCLASSEXW reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633577
    // Window class style constants: https://msdn.microsoft.com/en-us/library/windows/desktop/ff729176
    // LoadImage reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms648045
    // CreateSolidBrush reference: https://msdn.microsoft.com/en-us/library/dd183518
    // RegisterClassEx reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633587

    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
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

    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_FUSIONLEVELEDITOR);
    wcex.lpszClassName = className;
    wcex.hIconSm = static_cast<HICON>(LoadImage(instance, MAKEINTRESOURCE(IDI_SMALL), IMAGE_ICON, SM_CXSMICON, SM_CYSMICON, LR_DEFAULTCOLOR));
    if (!wcex.hIconSm)
        throw WindowsError("Failed to load small icon");

    ATOM&& registeredClass(RegisterClassEx(&wcex));
    if (!registeredClass)
        throw WindowsError("Failed to register main window class");
}

void Windows::createWindow(int cmdShow)
{
    // CreateWindowEx reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632680
    // Window style constants: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632600
    // Window extended style constants: https://msdn.microsoft.com/en-us/library/windows/desktop/ff700543

    // style & WS_VISIBLE shows the window after creation.
    // When x == CW_USEDEFAULT and y != CW_USEDEFAULT, y is used as the cmdShow parameter
    const DWORD exStyle{};
    const DWORD style(WS_TILEDWINDOW | WS_VISIBLE);
    const int x(CW_USEDEFAULT);
    const int y(cmdShow);
    const int width(CW_USEDEFAULT);
    const int height(CW_USEDEFAULT);
    HWND const windowParent{};
    HMENU const menu{};
    void* const param{};
    HWND window = CreateWindowEx(exStyle, className, titleString, style, x, y, width, height, windowParent, menu, instance, param);
    if (!window)
        WindowsError("Failed to create main window");
}

int Windows::eventLoop()
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

LRESULT CALLBACK Windows::windowPrecedure(HWND window, unsigned message, WPARAM wParam, LPARAM lParam)
{
    // Window precedure reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632593

    switch (message)
    {
    default:
        return DefWindowProc(window, message, wParam, lParam);
    
    // WM_COMMAND reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms647591
    case WM_COMMAND:
    {
        const WORD id = LOWORD(wParam);
        switch (id)
        {
        default:
            throw WindowsError("Unrecognised command identifier: " + std::to_string(id));

        case IDM_ABOUT:
        {
            // DialogBox reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms645452
            // Can use CreateDialog for a modeless version
            const std::intptr_t dialogRet(DialogBox(nullptr, MAKEINTRESOURCE(IDD_ABOUTBOX), window, about));
            if (dialogRet == 0 || dialogRet == -1)
                throw WindowsError("Failed to open about dialog box");

            break;
        }
        
        case IDM_EXIT:
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
        PostQuitMessage(EXIT_SUCCESS);
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

// Message handler for about box.
std::intptr_t CALLBACK Windows::about(HWND window, UINT message, WPARAM wParam, LPARAM)
{
    // Dialog procedure reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms645469
    // Dialog box reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632588

    switch (message)
    {
    default:
        return FALSE;

    // WM_INITDIALOG reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms645428
    case WM_INITDIALOG:
        return TRUE;

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

    return TRUE;
}
