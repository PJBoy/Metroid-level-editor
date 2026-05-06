module;

#include "../global.h"

export module window_layout;

export import window;

export namespace window_layout
{
struct FractionalDimension
{
    float length;
};

struct FixedDimension
{
    unsigned length;
};

struct DeducedDimension
{};

using Dimension = std::variant<FractionalDimension, FixedDimension, DeducedDimension>;

enum struct Direction
{
    horizontal,
    vertical
};

struct Window
{
    ::Window* p_window;
    Dimension dimension;
};
}

export struct WindowLayout
{
    std::vector<window_layout::Window> windows;
    window_layout::Direction direction;

    static WindowLayout makeRow(std::vector<window_layout::Window> windows);
    static WindowLayout makeColumn(std::vector<window_layout::Window> windows);

    void createWindows(unsigned width_total, unsigned height_total, unsigned origin_x, unsigned origin_y, const Window& windowParent);
};
