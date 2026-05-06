module;

#include "../global.h"

export module main_window;

export import window;

export import renderer;

import level_window;
import rom;
import window_layout;

export class MainWindow : public Window
{
    WindowLayout windowLayout;
    LevelWindow levelView;
    std::unique_ptr<Rom> p_rom;
    RendererFactory* p_rendererFactory;

public:
    MainWindow(Os& os, RendererFactory& rendererFactory, std::any os_arg);

    std::string_view getName() const override { return "MainWindow"sv; }
    void onDestroy() override;
    void openRom();
};
