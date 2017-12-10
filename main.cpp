#include "debug.h"
#include "os.h"
#include "os_windows.h"

#include "global.h"

#include <exception>

// WinMain reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633559
// wWinMain article: https://msdn.microsoft.com/en-us/library/windows/desktop/ff381406
int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t* cmdLine, int cmdShow)
try
{
    Windows windows(instance, cmdShow);
    Os& os(windows);

    return os.eventLoop();
}
catch (const std::exception& e) // Any other exception is completely unexpected and means the program is in a loopy state. I let the throw violate the noexept specification to avoid ensuring destructors are called
{
    DebugFile("debug_main_error.txt") << e.what() << '\n';
    return EXIT_FAILURE;
}
