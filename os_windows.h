#pragma once

#include "os.h"

#include "config.h"

#include "global.h"

#define OEMRESOURCE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>


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
        //     A static unsigned long style constant named `exStyle`
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
    
        inline void create(int x, int y, int width, int height, HWND parentWindow)
        try
        {
            // CreateWindowEx reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632680
            // Window style constants: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632600
            // Window extended style constants: https://msdn.microsoft.com/en-us/library/windows/desktop/ff700543

            const unsigned long exStyle(Derived::exStyle);
            const unsigned long style(Derived::style | WS_VISIBLE);
            HMENU const menu{};
            void* const param{};
            window = CreateWindowEx(exStyle, Derived::className, Derived::titleString, style, x, y, width, height, parentWindow, menu, windows.instance, param);
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

    template<typename Derived, n_t n_digits>
    class AddressEditWindow : public Window<Derived>
    {
        // Edit control reference: https://docs.microsoft.com/en-us/windows/win32/controls/edit-controls
        friend Window;

        const inline static wchar_t
            *const titleString = L"",
            *const className = WC_EDIT;

        const inline static unsigned long
            style{WS_CHILD | WS_TABSTOP},
            exStyle{};

    public:
        using Window<Derived>::Window;

        inline void create(int x, int y, int width, int height, HWND parentWindow)
        try
        {
            Window<Derived>::create(x, y, width, height, parentWindow);

            // SendMessage reference: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendmessage
            // WM_FONT reference: https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-setfont
            // EM_LIMITTEXT reference: https://docs.microsoft.com/en-us/windows/win32/controls/em-limittext
            SendMessage(Window<Derived>::window, WM_SETFONT, std::uintptr_t(p_windows->monospace), true);
            Edit_LimitText(Window<Derived>::window, n_digits);
        }
        LOG_RETHROW
    };

    template<typename Derived>
    class LabelWindow : public Window<Derived>
    {
        // Static control reference: https://docs.microsoft.com/en-us/windows/win32/controls/static-controls
        friend Window;

        const inline static wchar_t* const className = WC_STATIC;
        const inline static unsigned long
            style{WS_CHILD},
            exStyle{};

    public:
        using Window<Derived>::Window;

        inline void create(int x, int y, int width, int height, HWND parentWindow)
            try
        {
            Window<Derived>::create(x, y, width, height, parentWindow);

            // SendMessage reference: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendmessage

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
        const inline static unsigned long
            style{WS_CHILD | WS_HSCROLL | WS_VSCROLL},
            exStyle{WS_EX_WINDOWEDGE};

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
        const inline static unsigned long
            style{WS_CHILD | WS_HSCROLL | WS_VSCROLL | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT},
            exStyle{WS_EX_WINDOWEDGE};

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
            const inline static unsigned long
                style{WS_CHILD | WS_HSCROLL | WS_VSCROLL},
                exStyle{WS_EX_WINDOWEDGE};

            inline static SpritemapView* p_spritemapView;

            static LRESULT CALLBACK windowProcedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR lParam) noexcept;

        public:
            SpritemapView(Windows& windows);
        };

        class SpritemapTilesView : public Window<SpritemapTilesView>
        {
            friend Window;

            const inline static wchar_t
                *const titleString = L"",
                *const className = L"SpritemapTilesView";

            const inline static std::string classDescription{"spritemap tiles view"};
            const inline static unsigned long
                style{WS_CHILD | WS_BORDER},
                exStyle{WS_EX_WINDOWEDGE};

            inline static SpritemapTilesView* p_spritemapTilesView;

            static LRESULT CALLBACK windowProcedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR lParam) noexcept;

        public:
            SpritemapTilesView(Windows& windows);
        };

        class TilesAddressLabel : public LabelWindow<TilesAddressLabel>
        {
            friend Window;

            const inline static wchar_t* const titleString = L"Tiles";
            const inline static std::string classDescription{"tile address label"};
            inline static TilesAddressLabel* p_tilesAddressLabel;

        public:
            TilesAddressLabel(Windows& windows);
        };

        class PaletteAddressLabel : public LabelWindow<PaletteAddressLabel>
        {
            friend Window;

            const inline static wchar_t* const titleString = L"Palettes";
            const inline static std::string classDescription{"palette address label"};
            inline static PaletteAddressLabel* p_paletteAddressLabel;

        public:
            PaletteAddressLabel(Windows& windows);
        };

        class SpritemapAddressLabel : public LabelWindow<SpritemapAddressLabel>
        {
            friend Window;

            const inline static wchar_t* const titleString = L"Spritemap";
            const inline static std::string classDescription{"spritemap address label"};
            inline static SpritemapAddressLabel* p_spritemapAddressLabel;

        public:
            SpritemapAddressLabel(Windows& windows);
        };

        class TilesAddressInput : public AddressEditWindow<TilesAddressInput, 6>
        {
            friend Window;
            const inline static std::string classDescription{"tile address input"};
            inline static TilesAddressInput* p_tilesAddressInput;

        public:
            TilesAddressInput(Windows& windows);
        };

        class PaletteAddressInput : public AddressEditWindow<PaletteAddressInput, 6>
        {
            friend Window;
            const inline static std::string classDescription{"palette address input"};
            inline static PaletteAddressInput* p_paletteAddressInput;

        public:
            PaletteAddressInput(Windows& windows);
        };

        class SpritemapAddressInput : public AddressEditWindow<SpritemapAddressInput, 6>
        {
            friend Window;
            const inline static std::string classDescription{"spritemap address input"};
            inline static SpritemapAddressInput* p_spritemapAddressInput;

        public:
            SpritemapAddressInput(Windows& windows);
        };

        const inline static wchar_t
            *const titleString = L"Spritemap viewer",
            *const className = L"SpritemapViewer";

        const inline static std::string classDescription{"spritemap view"};
        const inline static unsigned long
            style{WS_TILEDWINDOW},
            exStyle{WS_EX_WINDOWEDGE};

        const inline static float
            y_ratio_spritemapView{1},
            x_ratio_spritemapView{2./3};

        const inline static unsigned
            y_length_spritemapTiles{0x200},
            x_length_spritemapTiles{0x100},
            y_length_addressLabel{19},
            x_length_addressLabel{96},
            y_length_addressInput{16},
            x_length_addressInput{96};

        inline static SpritemapViewer* p_spritemapViewer;

        std::unique_ptr<SpritemapView> p_spritemapView;
        std::unique_ptr<SpritemapTilesView> p_spritemapTilesView;
        std::unique_ptr<TilesAddressLabel> p_tilesAddressLabel;
        std::unique_ptr<PaletteAddressLabel> p_palettesAddressLabel;
        std::unique_ptr<SpritemapAddressLabel> p_spritemapAddressLabel;
        std::unique_ptr<TilesAddressInput> p_tilesAddressInput;
        std::unique_ptr<PaletteAddressInput> p_palettesAddressInput;
        std::unique_ptr<SpritemapAddressInput> p_spritemapAddressInput;
        HWND activeInput{};

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
    HFONT monospace;
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
