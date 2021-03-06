#include "config.h"
#include "os.h"
#include "os_windows.h"

#include "global.h"

#include <iostream>
#include <iterator>
#include <vector>

bool isArg(std::vector<std::string>& args, std::string arg)
{
    const auto it_arg(std::find(args.begin(), args.end(), (arg.size() == 1 ? "-" : "--") + arg));
    if (it_arg != args.end())
    {
        args.erase(it_arg);
        return true;
    }

    return false;
}

// WinMain reference: https://msdn.microsoft.com/en-us/library/windows/desktop/ms633559
// wWinMain article: https://msdn.microsoft.com/en-us/library/windows/desktop/ff381406
int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t* cmdLine, int cmdShow)
try
{
    Config config;
    Windows windows(instance, cmdShow, config);
    Os& os(windows);
    DebugFile::init(os.getDataDirectory());

    // get cmdline
    
    try
    {
        config.init(os.getDataDirectory());
        config.load();
    }
    catch (const std::exception& e)
    {
        DebugFile(DebugFile::warning) << LOG_INFO "Failed to load config, using default config: " << e.what() << '\n';
    }

    os.init();
    return os.eventLoop();
}
catch (const std::exception& e) // Any other exception is completely unexpected and means the program is in a loopy state. I let the throw violate the noexcept specification to avoid ensuring destructors are called
{
    DebugFile(DebugFile::error) << LOG_INFO << e.what() << '\n';
    return EXIT_FAILURE;
}
