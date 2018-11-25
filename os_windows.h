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
        HWND window;
        
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
        HWND window;

        RoomSelectorTree(Windows& windows) noexcept;
        RoomSelectorTree(const RoomSelectorTree&) = delete;
        RoomSelectorTree(RoomSelectorTree&&) = delete;
        RoomSelectorTree& operator=(const RoomSelectorTree&) = delete;
        RoomSelectorTree& operator=(RoomSelectorTree&&) = delete;
        void create(int x, int y, int width, int height);
        void destroy();
    };

    // Constants //
    const inline static wchar_t
        *const titleString = L"Metroid editor",
        *const className = L"MetroidLevelEditor";

    // Variables //
    inline static Windows* p_windows; // Provides access to this global state from callback functions
    HINSTANCE instance; // Identifies the module (the executable)
    HWND window; // Identifies the window (needed for child windows)
    int cmdShow; // Needed for createWindow
    HACCEL accelerators;
    Config& config; // Reference held to update recent files in response to file opening
    std::unique_ptr<LevelView> p_levelView;
    std::unique_ptr<RoomSelectorTree> p_roomSelectorTree;

    // Functions //
    void registerClass();
    void createWindow();
    void error(const std::wstring& errorText) const;
    void createChildWindows();
    void destroyChildWindows();
    void handleCommand(unsigned int id, bool isAccelerator);
    void openRom(std::filesystem::path filepath);
    static LRESULT CALLBACK windowProcedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR lParam) noexcept;
    static std::uintptr_t CALLBACK openRomHookProcedure(HWND window, unsigned message, std::uintptr_t, LONG_PTR lParam) noexcept;

public:
    Windows(HINSTANCE instance, int cmdShow, Config& config) noexcept;
    ~Windows() = default;

    void init() override;
    int eventLoop() override;
    std::filesystem::path getDataDirectory() const override;
    void error(const std::string & errorText) const override;
};
