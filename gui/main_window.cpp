#include "../global.h"

import main_window;

static MenuEntry makeMenu_file()
try
{
    MenuEntry menu(MenuEntry::makeSubmenu());
    menu.text = "File";
        
    {
        MenuEntry exit(MenuEntry::makeItem());
        exit.text = "Exit";
        exit.asItem().action = [](Window& window)
        {
            window.onDestroy();
        };
        menu.asSubmenu().entries.push_back(std::move(exit));
    }

    return menu;
}
LOG_RETHROW

static MenuEntry makeMenu_help()
try
{
    MenuEntry menu(MenuEntry::makeSubmenu());
    menu.text = "Help";
        
    {
        MenuEntry about(MenuEntry::makeItem());
        about.text = "About";
        about.isDisabled = true;
        menu.asSubmenu().entries.push_back(std::move(about));
    }

    return menu;
}
LOG_RETHROW

static Menu makeMenu()
try
{
    Menu menu;
    menu.entries.push_back(makeMenu_file());
    menu.entries.push_back(makeMenu_help());

    return menu;
}
LOG_RETHROW

MainWindow::MainWindow(Os& os, std::any os_arg)
try
    : Window(os)
{
    menu = makeMenu();
    p_os->spawnMainWindow(*this, "MainWindow", "Metroid level editor", std::move(os_arg));
}
LOG_RETHROW

void MainWindow::onDestroy()
try
{
    p_os->quit();
}
LOG_RETHROW
