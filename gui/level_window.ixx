module;

#include "../global.h"

export module level_window;

export import window;
export import rom;
export import renderer;

export class LevelWindow : public Window
{
    Rom* p_rom;
    std::unique_ptr<WindowRenderer> p_renderer;
    std::unique_ptr<renderer::Bitmap> p_bitmap;

public:
    LevelWindow() = default;
    LevelWindow(Os& os, Rom& rom);

    // Call this after create()
    void init(RendererFactory& rendererFactory);

    std::string_view getName() const override { return "LevelWindow"sv; }
    void onPaint() override;
};
