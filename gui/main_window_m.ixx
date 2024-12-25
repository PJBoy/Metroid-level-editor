module;

#include "../global.h"

export module main_window;

export import window;

export class MainWindow : public Window
{
public:
    MainWindow(Os& os, std::any arg);

    void onDestroy() override;
};
