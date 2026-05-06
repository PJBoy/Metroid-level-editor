#include "../global.h"

import window_layout;

using namespace window_layout;

WindowLayout WindowLayout::makeRow(std::vector<window_layout::Window> windows)
{
    WindowLayout row;
    row.windows = std::move(windows);
    row.direction = Direction::horizontal;
    return row;
}

WindowLayout WindowLayout::makeColumn(std::vector<window_layout::Window> windows)
{
    WindowLayout column;
    column.windows = std::move(windows);
    column.direction = Direction::vertical;
    return column;
}

static std::vector<unsigned> calculateLengths(unsigned length_total, std::span<const window_layout::Window> windows)
{
    const n_t n_windows = std::size(windows);
    std::vector<unsigned> lengths(n_windows);
    index_t i_deduced = -1;
    unsigned length_remaining = length_total;

    for (index_t i{}; i < n_windows; ++i)
    {
        const window_layout::Window& window = windows[i];
        unsigned& length = lengths[i];

        if (std::holds_alternative<FractionalDimension>(window.dimension))
            length = length_total * std::get<FractionalDimension>(window.dimension).length;
        else if (std::holds_alternative<FixedDimension>(window.dimension))
            length = std::get<FixedDimension>(window.dimension).length;
        else
        {
            i_deduced = i;
            length = 0;
        }

        if (length > length_remaining)
            throw std::runtime_error(LOG_INFO "Not enough space to place window");

        length_remaining -= length;
        lengths.push_back(std::move(length));
    }

    if (i_deduced != -1)
        lengths[i_deduced] = length_remaining;

    return lengths;
}

void WindowLayout::createWindows(unsigned width_total, unsigned height_total, unsigned origin_x, unsigned origin_y, const ::Window& windowParent)
{
    const n_t n_windows = std::size(windows);
    if (direction == Direction::horizontal)
    {
        const std::vector<unsigned> lengths = calculateLengths(width_total, windows);
        unsigned x = origin_x;
        for (index_t i{}; i < n_windows; ++i)
        {
            window_layout::Window& window = windows[i];
            unsigned width = lengths[i];

            window.p_window->create(width, height_total, x, origin_y, windowParent);
            x += width;
        }
    }
    else
    {
        const std::vector<unsigned> lengths = calculateLengths(height_total, windows);
        unsigned y = origin_y;
        for (index_t i{}; i < n_windows; ++i)
        {
            window_layout::Window& window = windows[i];
            unsigned height = lengths[i];

            window.p_window->create(width_total, height, origin_x, y, windowParent);
            y += height;
        }
    }
}
