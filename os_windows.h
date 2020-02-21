#pragma once

#include "os.h"

#include "config.h"
#include "matrix.h"

#include "global.h"

#define OEMRESOURCE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>

#include <array>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <variant>
#include <vector>


std::string toString(const std::wstring& from) noexcept;
std::wstring toWstring(const std::string& from) noexcept;


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
    class WindowBase;

    // Dynamically polymprhic window layout base class
    class WindowLayoutBase
    {
    public:
        using element_arg_types_t = std::tuple<double, int, std::monostate>;

        enum struct LengthType
        {
            fraction,
            fixed,
            deduced
        };

        enum struct Orientation
        {
            horizontal,
            vertical
        };

        virtual ~WindowLayoutBase() = default;

        virtual void create(HWND parentWindow, int parentWidth, int parentHeight, int x = 0, int y = 0) = 0;
        virtual void resize(int parentWidth, int parentHeight, int x = 0, int y = 0) = 0;
    };

    // Statically polymorphic window layout base class
    template<WindowLayoutBase::Orientation orientation>
    class WindowLayout : public WindowLayoutBase
    {
    public:
        // Public nested classes
        struct Element
        {
            using element_arg_t =
                std::variant
                <
                    std::tuple_element_t<toInt(LengthType::fraction), element_arg_types_t>, 
                    std::tuple_element_t<toInt(LengthType::fixed),    element_arg_types_t>, 
                    std::tuple_element_t<toInt(LengthType::deduced),  element_arg_types_t>
                >;

            element_arg_t arg;
            std::variant<WindowBase*, std::unique_ptr<WindowLayoutBase>> p_window;
        };

        // Static member variables
        constexpr static auto tag_fraction = std::in_place_index<toInt(LengthType::fraction)>;
        constexpr static auto tag_fixed    = std::in_place_index<toInt(LengthType::fixed)>;
        constexpr static auto tag_deduced  = std::in_place_index<toInt(LengthType::deduced)>;

    private:
        // Non-static member variables
        std::vector<Element> elements;
        unsigned margin;

    public:
        // Static member functions
        template<n_t n>
        static std::unique_ptr<WindowLayoutBase> make(Element (&&elements)[n], unsigned margin = 0)
        {
            return std::make_unique<WindowLayout>(std::move(elements), margin);
        }

        // Non-static member functions
        WindowLayout() = default;

        template<n_t n>
        explicit WindowLayout(Element (&&elements)[n], unsigned margin = 0)
        try
            : elements(std::make_move_iterator(std::begin(elements)), std::make_move_iterator(std::end(elements))), margin(margin)
        {}
        LOG_RETHROW

        void create(HWND parentWindow, int parentWidth, int parentHeight, int x = 0, int y = 0) override
        try
        {
            createOrResize<false>(parentWindow, parentWidth, parentHeight, x, y);
        }
        LOG_RETHROW

        void resize(int parentWidth, int parentHeight, int x = 0, int y = 0) override
        try
        {
            createOrResize<true>({}, parentWidth, parentHeight, x, y);
        }
        LOG_RETHROW

        template<bool resize>
        void createOrResize(HWND parentWindow, int parentWidth, int parentHeight, int x = 0, int y = 0)
        try
        {
            if constexpr (resize)
                (void)parentWindow;

            if constexpr (orientation == WindowLayoutBase::Orientation::horizontal)
                parentWidth -= int(margin * (std::size(elements) - 1));
            else
                parentHeight -= int(margin * (std::size(elements) - 1));

            for (const Element& element : elements)
            {
                int width, height;

                if constexpr (orientation == WindowLayoutBase::Orientation::horizontal)
                {
                    if (element.arg.index() == toInt(LengthType::fraction))
                        width = int(parentWidth * std::get<double>(element.arg));
                    else
                        width = std::get<int>(element.arg);

                    height = parentHeight;
                }
                else
                {
                    width = parentWidth;
                    if (element.arg.index() == toInt(LengthType::fraction))
                        height = int(parentHeight * std::get<double>(element.arg));
                    else
                        height = std::get<int>(element.arg);
                }

                if (std::holds_alternative<WindowBase*>(element.p_window))
                    if constexpr (resize)
                        std::get<WindowBase*>(element.p_window)->resize(x, y, width, height);
                    else
                        std::get<WindowBase*>(element.p_window)->create(x, y, width, height, parentWindow);
                else
                    if constexpr (resize)
                        std::get<std::unique_ptr<WindowLayoutBase>>(element.p_window)->resize(width, height, x, y);
                    else
                        std::get<std::unique_ptr<WindowLayoutBase>>(element.p_window)->create(parentWindow, width, height, x, y);

                if (element.arg.index() == toInt(LengthType::deduced))
                {
                    if (!std::holds_alternative<WindowBase*>(element.p_window))
                        throw std::runtime_error("Using tag_deduced with a WindowLayout is not supported");

                    if constexpr (orientation == WindowLayoutBase::Orientation::horizontal)
                        x += std::get<WindowBase*>(element.p_window)->getWidth() + margin;
                    else
                        y += std::get<WindowBase*>(element.p_window)->getHeight() + margin;
                }
                else
                {
                    if constexpr (orientation == WindowLayoutBase::Orientation::horizontal)
                        x += width + margin;
                    else
                        y += height + margin;
                }
            }
        }
        LOG_RETHROW
    };

    using WindowRow = WindowLayout<WindowLayoutBase::Orientation::horizontal>;
    using WindowColumn = WindowLayout<WindowLayoutBase::Orientation::vertical>;

    // Dynamically polymprhic window base class
    class WindowBase
    {
        WindowBase(const WindowBase&) = delete;
        WindowBase(WindowBase&&) = delete;
        WindowBase& operator=(const WindowBase&) = delete;
        WindowBase& operator=(WindowBase&&) = delete;

    public:
        HWND window{};

        WindowBase() = default;

        virtual void create(int x, int y, int width, int height, HWND parentWindow) = 0;
        virtual void resize(int x, int y, int width, int height) = 0;
        virtual int getWidth() const = 0;
        virtual int getHeight() const = 0;
    };

    // Statically polymorphic window base class
    template<typename Derived>
    class Window : public WindowBase
    {
        // Derived class must have:
        //     `static wchar_t* titleString`
        //     `static wchar_t* className`
        //     `static unsigned long exStyle`
        //     `static unsigned long style`
        //     A static narrow string constant named `classDescription`

        // Derived class may have:
        //     `static LRESULT CALLBACK windowProcedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR lParam)`

        struct detail
        {
            template<typename T, typename = void>
            constexpr static bool hasWindowProcedure{};

            template<typename T>
            constexpr static bool hasWindowProcedure<T, std::void_t<decltype(T::windowProcedure)>>{true};
        };

        template<typename T>
        constexpr bool static hasWindowProcedure{detail::template hasWindowProcedure<T>};

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
    
        inline void create(int x, int y, int width, int height, HWND parentWindow) override
        try
        {
            // CreateWindowEx reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632680
            // Window style constants: https://msdn.microsoft.com/en-us/library/windows/desktop/ms632600
            // Window extended style constants: https://msdn.microsoft.com/en-us/library/windows/desktop/ff700543

            if (window)
                return;

            const unsigned long exStyle(Derived::exStyle);
            const unsigned long style(Derived::style | WS_VISIBLE);
            HMENU const menu{};
            void* const param{};
            window = CreateWindowEx(exStyle, Derived::className, Derived::titleString, style, x, y, width, height, parentWindow, menu, windows.instance, param);
            if (!window)
                throw WindowsError(LOG_INFO "Failed to create "s + Derived::classDescription + " window"s);
        }
        LOG_RETHROW
    
        inline void resize(int x, int y, int width, int height) override
        try
        {
            // MoveWindow reference: https://docs.microsoft.com/en-gb/windows/win32/api/winuser/nf-winuser-movewindow

            if (!MoveWindow(window, x, y, width, height, true))
                throw WindowsError(LOG_INFO "Failed to move/resize "s + Derived::classDescription + " window"s);
        }
        LOG_RETHROW

        inline int getWidth() const
        {
            // RECT reference: https://msdn.microsoft.com/en-us/library/windows/desktop/dd162897
            // GetClientRec reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633503

            RECT rect;
            if (!GetClientRect(window, &rect))
                throw WindowsError(LOG_INFO "Failed to get size of client area of "s + Derived::classDescription + " window"s);

            return rect.right;
        }

        inline int getHeight() const
        {
            // RECT reference: https://msdn.microsoft.com/en-us/library/windows/desktop/dd162897
            // GetClientRec reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633503

            RECT rect;
            if (!GetClientRect(window, &rect))
                throw WindowsError(LOG_INFO "Failed to get size of client area of "s + Derived::classDescription + " window"s);

            return rect.bottom;
        }
    
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

        inline void invalidate()
        try
        {
            // InvalidateRect reference: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-invalidaterect
            if (!InvalidateRect(window, nullptr, true))
                throw WindowsError(LOG_INFO "Failed to invalidate "s + Derived::classDescription + " window"s);
        }
        LOG_RETHROW
    };

    // Window subclasses
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

        inline unsigned long getValue() const
        try
        {
            // Edit_GetText reference: https://docs.microsoft.com/en-us/windows/win32/api/windowsx/nf-windowsx-edit_gettext

            std::wstring text(n_digits + 1, L'\0');
            Edit_GetText(Window<Derived>::window, std::data(text), int(std::size(text)));
            return std::stoul(text, nullptr, 0x10);
        }
        LOG_RETHROW

        inline bool isEmpty() const
        try
        {
            // Edit_GetTextLength reference: https://docs.microsoft.com/en-us/windows/win32/api/windowsx/nf-windowsx-edit_gettextlength

            SetLastError(ERROR_SUCCESS);
            const int length(Edit_GetTextLength(Window<Derived>::window));
            if (GetLastError())
                throw WindowsError(LOG_INFO "Failed to get "s + Derived::classDescription + " edit box length"s);

            return length == 0;
        }
        LOG_RETHROW
    };

    class StatusBarWindow : public Window<StatusBarWindow>
    {
        // Status bar reference: https://docs.microsoft.com/en-us/windows/win32/controls/status-bar-reference
        friend Window;

        const inline static wchar_t
            *const titleString = L"",
            *const className = STATUSCLASSNAME;

        const inline static std::string classDescription{"status bar"};
        const inline static unsigned long
            style{WS_CHILD},
            exStyle{};

    public:
        using Window::Window;

        inline void drawText(std::wstring text)
        {
            // SendMessage reference: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendmessage
            // InvalidateRect reference: https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-invalidaterect
            // WM_SETTEXT reference: https://docs.microsoft.com/en-gb/windows/win32/winmsg/wm-settext

            SendMessage(window, WM_SETTEXT, {}, LONG_PTR(std::data(text)));
        }
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
            friend AddressEditWindow;

            const inline static std::string classDescription{"tile address input"};
            inline static TilesAddressInput* p_tilesAddressInput;

        public:
            TilesAddressInput(Windows& windows);
        };

        class PaletteAddressInput : public AddressEditWindow<PaletteAddressInput, 6>
        {
            friend Window;
            friend AddressEditWindow;

            const inline static std::string classDescription{"palette address input"};
            inline static PaletteAddressInput* p_paletteAddressInput;

        public:
            PaletteAddressInput(Windows& windows);
        };

        class SpritemapAddressInput : public AddressEditWindow<SpritemapAddressInput, 6>
        {
            friend Window;
            friend AddressEditWindow;

            const inline static std::string classDescription{"spritemap address input"};
            inline static SpritemapAddressInput* p_spritemapAddressInput;

        public:
            SpritemapAddressInput(Windows& windows);
        };

        
        class TilesDestAddressLabel : public LabelWindow<TilesDestAddressLabel>
        {
            friend Window;

            const inline static wchar_t* const titleString = L" -> ";
            const inline static std::string classDescription{"tile dest address label"};
            inline static TilesDestAddressLabel* p_tilesDestAddressLabel;

        public:
            TilesDestAddressLabel(Windows& windows);
        };

        class PaletteDestAddressLabel : public LabelWindow<PaletteDestAddressLabel>
        {
            friend Window;

            const inline static wchar_t* const titleString = L" -> ";
            const inline static std::string classDescription{"palette dest address label"};
            inline static PaletteDestAddressLabel* p_paletteDestAddressLabel;

        public:
            PaletteDestAddressLabel(Windows& windows);
        };

        
        class TilesDestAddressInput : public AddressEditWindow<TilesDestAddressInput, 4>
        {
            friend Window;
            friend AddressEditWindow;

            const inline static std::string classDescription{"tile dest address input"};
            inline static TilesDestAddressInput* p_tilesDestAddressInput;

        public:
            TilesDestAddressInput(Windows& windows);
        };

        class PaletteDestAddressInput : public AddressEditWindow<PaletteDestAddressInput, 4>
        {
            friend Window;
            friend AddressEditWindow;

            const inline static std::string classDescription{"palette dest address input"};
            inline static PaletteDestAddressInput* p_paletteDestAddressInput;

        public:
            PaletteDestAddressInput(Windows& windows);
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
            x_length_destAddressLabel{16},
            y_length_addressInput{16},
            x_length_addressInput{96};

        inline static SpritemapViewer* p_spritemapViewer;

        std::unique_ptr<StatusBarWindow> p_statusBar;
        std::unique_ptr<SpritemapView> p_spritemapView;
        std::unique_ptr<SpritemapTilesView> p_spritemapTilesView;
        std::unique_ptr<TilesAddressLabel> p_tilesAddressLabel;
        std::unique_ptr<PaletteAddressLabel> p_palettesAddressLabel;
        std::unique_ptr<SpritemapAddressLabel> p_spritemapAddressLabel;
        std::unique_ptr<TilesAddressInput> p_tilesAddressInput;
        std::unique_ptr<PaletteAddressInput> p_palettesAddressInput;
        std::unique_ptr<SpritemapAddressInput> p_spritemapAddressInput;
        std::unique_ptr<TilesDestAddressInput> p_tilesDestAddressInput;
        std::unique_ptr<PaletteDestAddressInput> p_palettesDestAddressInput;
        std::unique_ptr<TilesDestAddressLabel> p_tilesDestAddressLabel;
        std::unique_ptr<PaletteDestAddressLabel> p_palettesDestAddressLabel;
        std::unique_ptr<WindowLayoutBase> p_windowLayout;
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
        y_ratio_roomSelectorTree{1};

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
