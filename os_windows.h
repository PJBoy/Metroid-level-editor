#pragma once

#include "os.h"

#include "config.h"

#include "global.h"

#define OEMRESOURCE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class Windows : public Os
{
    // Constants //
    const inline static wchar_t
        *titleString = L"Fusion editor",
        *className = L"FusionLevelEditor";

    // Variables //
    inline static Windows* p_windows; // Provides access to this global state from callback functions
    HINSTANCE instance; // Identifies the module (the executable)
    int cmdShow; // Needed for createWindow
    HACCEL accelerators;
    Config& config; // Reference held to update recent files in response to file opening

    // Functions //
    void registerClass();
    void createWindow();
    friend LRESULT CALLBACK windowPrecedure(HWND window, unsigned message, std::uintptr_t wParam, LONG_PTR lParam) noexcept;
    friend std::uintptr_t CALLBACK openRomHookPrecedure(HWND window, unsigned message, std::uintptr_t, LONG_PTR lParam) noexcept;

public:
    Windows(HINSTANCE instance, int cmdShow, Config& config) noexcept;
    ~Windows() = default;

    void init() override;
    int eventLoop() override;
    std::filesystem::path getDataDirectory() const override;
    void error(const std::wstring & errorText) const override;
};
