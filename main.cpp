// Due to system headers playing poorly with modules, all system header includes must go before any module imports
#include <cstdlib> // for EXIT_FAILURE
#include "os_windows.h"

#include "global.h"

import config;
import os;

int main_common(Os& os)
try
{
    DebugFile::init(os.getDataDirectory());

    Config config(os.getDataDirectory());
    try
    {
        config.load();
    }
    catch (const std::exception& e)
    {
        DebugFile(DebugFile::warning) << LOG_INFO "Failed to load config, using default config: " << e.what() << '\n';
    }

    os.init(config);

    return os.eventLoop();
}
LOG_RETHROW

// WinMain reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633559
// wWinMain article: https://msdn.microsoft.com/en-us/library/windows/desktop/ff381406
int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t* cmdLine, int cmdShow)
try
{
    Windows windows(instance, cmdShow);

    return main_common(windows);
}
catch (const std::exception& e)
{
    DebugFile(DebugFile::error) << LOG_INFO << e.what() << '\n';
    return EXIT_FAILURE;
}
