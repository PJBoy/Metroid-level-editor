#include "os.h"
#include "os_windows.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// WinMain reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633559
// wWinMain article: https://msdn.microsoft.com/en-us/library/windows/desktop/ff381406
int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t* cmdLine, int cmdShow)
{
    Windows windows(instance, cmdShow);
    Os& os(windows);

    return os.eventLoop();
}
