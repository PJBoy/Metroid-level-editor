#include "../global.h"

import window;


MenuEntry MenuEntry::makeItem()
try
{
    MenuEntry entry;
    entry.entry = MenuItem();
    return entry;
}
LOG_RETHROW

MenuEntry MenuEntry::makeSubmenu()
try
{
    MenuEntry entry;
    entry.entry = Menu();
    return entry;
}
LOG_RETHROW

bool MenuEntry::isSubmenu() const
try
{
    return std::holds_alternative<Menu>(entry);
}
LOG_RETHROW


Window::Window(Os& os)
    : p_os(&os)
{}

void Window::onDestroy()
{}
