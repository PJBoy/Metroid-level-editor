module;

#include "../global.h"

export module window;

import os;

export struct MenuItem
{
    std::move_only_function<void(class Window&)> action;
};

export struct Menu
{
    std::vector<struct MenuEntry> entries;
};

export struct MenuEntry
{
    static MenuEntry makeItem();
    static MenuEntry makeSubmenu();

    std::string text;
    bool isDisabled{};

private:
    std::variant<MenuItem, Menu> entry;
    
public:
    bool isSubmenu() const;
    
    template<typename Self>
    ForwardLike<MenuItem, Self> asItem(this Self&& self);
    
    template<typename Self>
    ForwardLike<Menu, Self> asSubmenu(this Self&& self);
};

export class Window
{
protected:
    Os* p_os;

public:
    std::optional<Menu> menu;

    explicit Window(Os& os);

    virtual void onDestroy();
};

template<typename Self>
ForwardLike<MenuItem, Self> MenuEntry::asItem(this Self&& self)
try
{
    return std::forward_like<Self>(std::get<MenuItem>(self.entry));
}
LOG_RETHROW

template<typename Self>
ForwardLike<Menu, Self> MenuEntry::asSubmenu(this Self&& self)
try
{
    return std::forward_like<Self>(std::get<Menu>(self.entry));
}
LOG_RETHROW
