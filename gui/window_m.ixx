module;

#include "../global.h"

export module window;

export import os;

export class Window
{
protected:
    Os* p_os;

public:
    explicit Window(Os& os);

    virtual void onDestroy();
};
