#pragma once

#include "os.h"

#include "global.h"

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

    // Functions //
    void registerClass();
    void createWindow(int cmdShow);
    int eventLoop();

public:
    Windows(HINSTANCE instance, int cmdShow);
    ~Windows() = default;
};
