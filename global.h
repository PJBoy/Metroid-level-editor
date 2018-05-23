#pragma once

#include <cinttypes>
#include <string>
#include <type_traits>

#define STRINGIFY(x) #x
#define APPLY(f, x) f(x)

using namespace std::literals;
using std::size_t;
using index_t = size_t;
using n_t     = size_t;
using byte     = std::uint8_t;
using halfword = std::uint16_t;
using word     = std::uint32_t;

#include "debug.h"

template<typename T, std::enable_if_t<std::is_enum_v<T>>* = nullptr>
constexpr auto toInt(T v) noexcept
{
    return static_cast<std::underlying_type_t<T>>(v);
}
