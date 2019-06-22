#pragma once

#include "os.h"

#include "config.h"

#include "global.h"

#define OEMRESOURCE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>


class WindowsError : public std::runtime_error
{
    // GetLastError reference: https://msdn.microsoft.com/en-gb/library/windows/desktop/ms679360

protected:
    static std::wstring getErrorMessage(unsigned long errorId);
    static std::string makeMessage() noexcept;
    static std::string makeMessage(const std::string& extraMessage) noexcept;
    static std::string makeMessage(unsigned long errorId) noexcept;
    static std::string makeMessage(unsigned long errorId, const std::string& extraMessage) noexcept;

public:
    WindowsError() noexcept;
    WindowsError(unsigned long errorId) noexcept;
    WindowsError(const std::string& extraMessage) noexcept;
    WindowsError(unsigned long errorId, const std::string& extraMessage) noexcept;
};

class CommonDialogError : public std::runtime_error
{
    // CommDlgExtendedError reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646916

protected:
    static std::string getErrorMessage(unsigned long errorId);
    static std::string makeMessage() noexcept;
    static std::string makeMessage(const std::string& extraMessage) noexcept;
    static std::string makeMessage(unsigned long errorId) noexcept;
    static std::string makeMessage(unsigned long errorId, const std::string& extraMessage) noexcept;

public:
    CommonDialogError() noexcept;
    CommonDialogError(unsigned long errorId) noexcept;
    CommonDialogError(const std::string& extraMessage) noexcept;
    CommonDialogError(unsigned long errorId, const std::string& extraMessage) noexcept;
};


class Windows final : public Os
{
    template<typename Derived>
    class Window
    {
        // Derived class must have:
        //     A static wide string constant named `titleString`
        //     A static wide string constant named `className`
        //     A static narrow string constant named `classDescription`
        //     A static unsigned long style constant named `style`

        // Derived class may have:
        //     A static window procedure named `windowProcedure`
    
        Window(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(const Window&) = delete;
        Window& operator=(Window&&) = delete;

        struct detail
        {
            template<typename T, typename = void>
            struct hasWindowProcedure
                : std::false_type
            {};

            template<typename T>
            struct hasWindowProcedure<T, std::void_t<decltype(T::windowProcedure)>>
                : std::true_type
            {};
        };

        template<typename T>
        constexpr bool static hasWindowProcedure{detail::template hasWindowProcedure<T>::value};

    protected:
        Windows& windows;

    public:
        HWND window{};

        Window(Windows& windows)
        try
            : windows(windows)
        {
            // Window class article: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633574
            // WNDCLASSEXW reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633577
            // CreateSolidBrush reference: https://msdn.microsoft.com/en-us/library/dd183518
            // RegisterClassEx reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633587
            // Window class style constants: https://msdn.microsoft.com/en-us/library/windows/desktop/ff729176

            if constexpr (hasWindowProcedure<Derived>)
            {
                WNDCLASSEXW wcex{};
                wcex.cbSize = sizeof(wcex);
                wcex.style = CS_HREDRAW | CS_VREDRAW; // Redraw the entire window if a movement or size adjustment changes the width or height of the client area
                wcex.lpfnWndProc = Derived::windowProcedure;
                wcex.hInstance = windows.instance;
                wcex.hbrBackground = CreateSolidBrush(0x000000);
                if (!wcex.hbrBackground)
                    throw WindowsError(LOG_INFO "Failed to create background brush");

                wcex.lpszClassName = Derived::className;
                ATOM registeredClass(RegisterClassEx(&wcex));
                if (!registeredClass)
                    throw WindowsError(LOG_INFO "Failed to register "s + Derived::classDescription + " window class"s);
            }
        }
        LOG_RETHROW
    
        inline void create(int x, int y, int width, int height, HWND hwnd)
        try
        {
            // CreateWindowEx reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632680
            // Window style constants: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632600
            // Window extended style constants: https://msdn.microsoft.com/en-us/library/windows/desktop/ff700543

            const unsigned long exStyle(WS_EX_WINDOWEDGE);
            const unsigned long style(Derived::style | WS_VISIBLE);
            HMENU const menu{};
            void* const param{};
            window = CreateWindowEx(exStyle, Derived::className, Derived::titleString, style, x, y, width, height, hwnd, menu, windows.instance, param);
            if (!window)
                throw WindowsError(LOG_INFO "Failed to create "s + Derived::classDescription + " window"s);
        }
        LOG_RETHROW
    
        inline void destroy()
        try
        {
            // DestroyWindow reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632682

            if (!window)
                return;

            if (!DestroyWindow(window))
                throw WindowsError(LOG_INFO "Failed to destroy "s + Derived::classDescription + " window"s);

            window = nullptr;
        }
        LOG_RETHROW
    };

    class LevelView : public Window<LevelView>
    {
        friend Window;

        const inline static wchar_t
            *const titleString = L"",
            *const className = L"LevelViewer";
        
        const inline static std::string classDescription{"level viewer"};
        const inline static unsigned long style{WS_CHILD | WS_HSCROLL | WS_VSCROLL};

        inline static LevelView* p_levelView;

        static LRESULT CALLBACK windowProcedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR lParam) noexcept;

    public:
        LevelView(Windows& windows);
    };

    class RoomSelectorTree : public Window<RoomSelectorTree>
    {
        // Tree view control reference: https://docs.microsoft.com/en-us/windows/desktop/controls/tree-view-control-reference
        friend Window;

        const inline static wchar_t
            *const titleString = L"",
            *const className = WC_TREEVIEW;

        const inline static std::string classDescription{"room selector tree"};
        const inline static unsigned long style{WS_CHILD | WS_HSCROLL | WS_VSCROLL | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT};

        void insertRoomList(const std::vector<Rom::RoomList>& roomLists, HTREEITEM parent = TVI_ROOT);

    public:
        using Window::Window;
    
        inline void create(int x, int y, int width, int height, HWND hwnd)
        try
        {
            Window::create(x, y, width, height, hwnd);
            insertRoomList(windows.p_rom->getRoomList());
        }
        LOG_RETHROW
    };

    class SpritemapViewer : public Window<SpritemapViewer>
    {
        friend Window;

        class SpritemapView : public Window<SpritemapView>
        {
            friend Window;

            const inline static wchar_t
                *const titleString = L"",
                *const className = L"SpritemapView";

            const inline static std::string classDescription{"spritemap view"};
            const inline static unsigned long style{WS_CHILD | WS_HSCROLL | WS_VSCROLL};

            inline static SpritemapView* p_spritemapView;

            static LRESULT CALLBACK windowProcedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR lParam) noexcept;

        public:
            SpritemapView(Windows& windows);
        };

        const inline static wchar_t
            *const titleString = L"Spritemap viewer",
            *const className = L"SpritemapViewer";

        const inline static std::string classDescription{"spritemap view"};
        const inline static unsigned long style{WS_TILEDWINDOW};

        const inline static float
            y_ratio_spritemapView{1},
            x_ratio_spritemapView{2./3};

        inline static SpritemapViewer* p_spritemapViewer;
        std::unique_ptr<SpritemapView> p_spritemapView;

        void createChildWindows();

        static LRESULT CALLBACK windowProcedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR lParam) noexcept;

    public:
        SpritemapViewer(Windows& windows);

        void create();
    };

    // Constants //
    const inline static wchar_t
        *const titleString = L"Metroid editor",
        *const className = L"MetroidLevelEditor";

    const inline static float
        y_ratio_levelEditor{1},
        x_ratio_levelEditor{2./3},
        y_ratio_roomSelectorTree{1},
        x_ratio_roomSelectorTree{1./3};

    // Variables //
    inline static Windows* p_windows; // Provides access to this global state from callback functions
    HINSTANCE instance; // Identifies the module (the executable)
    HWND window; // Identifies the window (needed for child windows)
    int cmdShow; // Needed for createWindow
    HACCEL accelerators;
    Config& config; // Reference held to update recent files in response to file opening
    std::unique_ptr<LevelView> p_levelView;
    std::unique_ptr<RoomSelectorTree> p_roomSelectorTree;
    std::unique_ptr<SpritemapViewer> p_spritemapViewer;

    // Functions //
    void registerClass();
    void createWindow();
    void error(const std::wstring& errorText) const;
    void createChildWindows();
    void destroyChildWindows();
    void handleCommand(unsigned int id, bool isAccelerator);
    void openRom(std::filesystem::path filepath);
    void updateLevelViewScrollbarDimensions();

    static LRESULT CALLBACK windowProcedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR lParam) noexcept;
    static std::uintptr_t CALLBACK openRomHookProcedure(HWND window, unsigned message, std::uintptr_t, LONG_PTR lParam) noexcept;

public:
    Windows(HINSTANCE instance, int cmdShow, Config& config) noexcept;
    ~Windows() = default;

    void init() override;
    int eventLoop() override;
    std::filesystem::path getDataDirectory() const override;
    void error(const std::string& errorText) const override;
};
