#include "global.h"

import main;

import main_window;

int main_common(Os& os, std::any main_window_arg)
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

    MainWindow mainWindow(os, std::move(main_window_arg));

    return os.eventLoop();
}
LOG_RETHROW
