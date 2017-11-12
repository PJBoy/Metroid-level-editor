#pragma once

#include "os.h"

#define OEMRESOURCE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class Windows : public Os
{
    // Constants //
    inline static const wchar_t
        *titleString = L"Fusion editor",
        *className = L"FusionLevelEditor";

    // Variables //
    HINSTANCE instance; // Identifies the module (the executable)
    HACCEL accelerators;

    // Callbacks //
    static INT_PTR CALLBACK about(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK windowPrecedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    // Functions //
    void registerClass();
    void createWindow(int cmdShow);
    int eventLoop();

public:
    Windows(HINSTANCE instance, int cmdShow);
    ~Windows() = default;
};
