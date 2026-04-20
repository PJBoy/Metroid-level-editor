module;

#include "../global.h"

export module window_layout;

export struct FractionalDimension
{
    float length;
};

export struct FixedDimension
{
    unsigned length;
};

export struct DeducedDimension
{};

export using Dimension = std::variant<FractionalDimension, FixedDimension, DeducedDimension>;

export enum struct Direction
{
    horizontal,
    vertical
};

export struct WindowLayout
{
    std::vector<Dimension> dimensions;
    Direction direction;

    static WindowLayout makeRow(std::vector<Dimension> dimensions)
    {
        WindowLayout row;
        row.dimensions = std::move(dimensions);
        row.direction = Direction::horizontal;
        return row;
    }

    static WindowLayout makeColumn(std::vector<Dimension> dimensions)
    {
        WindowLayout column;
        column.dimensions = std::move(dimensions);
        column.direction = Direction::vertical;
        return column;
    }
};
