#include "../global.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdlib> // for EXIT_FAILURE

import main;
import os_windows;

// WinMain reference: https://learn.microsoft.com/en-gb/windows/win32/api/winbase/nf-winbase-winmain
// wWinMain article: https://learn.microsoft.com/en-gb/windows/win32/learnwin32/winmain--the-application-entry-point
int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t* cmdLine, Windows::MainWindowArg_t cmdShow)
try
{
    Windows windows(instance);

    return main_common(windows, std::move(cmdShow));
}
catch (const std::exception& e)
{
    DebugFile(DebugFile::error) << LOG_INFO << e.what() << '\n';
    return EXIT_FAILURE;
}
