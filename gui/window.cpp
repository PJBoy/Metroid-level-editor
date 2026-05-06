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

void Window::onPaint()
{}

void Window::create(unsigned width, unsigned height, unsigned origin_x, unsigned origin_y, const Window& windowParent)
try
{
    p_os->spawnWindow(*this, width, height, origin_x, origin_y, windowParent);
}
LOG_RETHROW
