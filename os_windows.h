#pragma once

#include "os.h"

#include "config.h"

#include "global.h"

#define OEMRESOURCE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>

class Windows final : public Os
{
    class LevelView
    {
        const inline static wchar_t* const className = L"LevelViewer";

        inline static LevelView* p_levelView;
        Windows& windows;

        static LRESULT CALLBACK windowProcedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR lParam) noexcept;

    public:
        HWND window{};
        
        LevelView(Windows& windows);
        LevelView(const LevelView&) = delete;
        LevelView(LevelView&&) = delete;
        LevelView& operator=(const LevelView&) = delete;
        LevelView& operator=(LevelView&&) = delete;
        void create(int x, int y, int width, int height);
        void destroy();
    };

    class RoomSelectorTree
    {
        // Tree view control reference: https://docs.microsoft.com/en-us/windows/desktop/controls/tree-view-control-reference
        Windows& windows;

        void insertRoomList(const std::vector<Rom::RoomList>& roomLists, HTREEITEM parent);

    public:
        HWND window{};

        RoomSelectorTree(Windows& windows) noexcept;
        RoomSelectorTree(const RoomSelectorTree&) = delete;
        RoomSelectorTree(RoomSelectorTree&&) = delete;
        RoomSelectorTree& operator=(const RoomSelectorTree&) = delete;
        RoomSelectorTree& operator=(RoomSelectorTree&&) = delete;
        void create(int x, int y, int width, int height);
        void destroy();
    };

    class SpritemapViewer
    {
        class SpritemapView
        {
            const inline static wchar_t* const className = L"SpritemapView";

            inline static SpritemapView* p_spritemapView;
            Windows& windows;

            static LRESULT CALLBACK windowProcedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR lParam) noexcept;

        public:
            HWND window{};

            SpritemapView(Windows& windows);
            SpritemapView(const SpritemapView&) = delete;
            SpritemapView(SpritemapView&&) = delete;
            SpritemapView& operator=(const SpritemapView&) = delete;
            SpritemapView& operator=(SpritemapView&&) = delete;
            void create(int x, int y, int width, int height, HWND hwnd);
            void destroy();
        };

        const inline static wchar_t
            *const titleString = L"Spritemap viewer",
            *const className = L"SpritemapViewer";

        const inline static float
            y_ratio_spritemapView{1},
            x_ratio_spritemapView{2./3};

        inline static SpritemapViewer* p_spritemapViewer;
        Windows& windows;
        std::unique_ptr<SpritemapView> p_spritemapView;

        void createChildWindows();

        static LRESULT CALLBACK windowProcedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR lParam) noexcept;

    public:
        HWND window{};

        SpritemapViewer(Windows& windows);
        SpritemapViewer(const SpritemapViewer&) = delete;
        SpritemapViewer(SpritemapViewer&&) = delete;
        SpritemapViewer& operator=(const SpritemapViewer&) = delete;
        SpritemapViewer& operator=(SpritemapViewer&&) = delete;
        void create();
        void destroy();
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
