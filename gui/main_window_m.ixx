module;

#include "../global.h"

export module main_window;

export import window;
export import window_layout;
// export import rom;

export class MainWindow : public Window
{
    WindowLayout windowLayout;
    // std::unique_ptr<Rom> p_rom;

public:
    MainWindow(Os& os, std::any os_arg);

    void onDestroy() override;
    void openRom();
};
