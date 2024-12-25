#include "../global.h"

import main_window;

MainWindow::MainWindow(Os& os, std::any arg)
    : Window(os)
{
    p_os->spawnMainWindow(*this, "MainWindow", "Metroid level editor", std::move(arg));
}

void MainWindow::onDestroy()
{
    p_os->quit();
}
