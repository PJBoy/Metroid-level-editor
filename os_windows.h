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
    static Windows* p_windows; // Requires out-of-header definition
    HINSTANCE instance; // Identifies the module (the executable)
    int cmdShow; // Needed for createWindow
    HACCEL accelerators;
    Config& config; // Reference held to update recent files in response to file opening

    // Functions //
    void registerClass();
    void createWindow();
    friend std::intptr_t CALLBACK windowPrecedure(HWND window, unsigned message, std::uintptr_t wParam, std::intptr_t lParam) noexcept;

public:
    Windows(HINSTANCE instance, int cmdShow, Config& config) noexcept;
    ~Windows() = default;

    void init() override;
    int eventLoop() override;
    std::experimental::filesystem::path getDataDirectory() const override;
};
