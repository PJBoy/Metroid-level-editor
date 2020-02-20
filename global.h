#pragma once

#include <cstddef>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

#define STRINGIFY(x) #x
#define APPLY(f, x) f(x)

using namespace std::literals;
using std::size_t;
using index_t = size_t;
using n_t     = size_t;

#include "debug.h"

template<typename T, std::enable_if_t<std::is_enum_v<T>>* = nullptr>
constexpr auto toInt(T v) noexcept
{
    return static_cast<std::underlying_type_t<T>>(v);
}

template<typename T, typename Deleter>
constexpr std::unique_ptr<T, Deleter> makeUniquePtr(T* p, Deleter&& deleter)
{
    return std::unique_ptr<T, decltype(deleter)>(p, std::forward<Deleter>(deleter));
}

template<typename T>
constexpr std::string toHexString(T v, n_t n_bytes = sizeof(T))
{
    std::ostringstream out;
    out << std::hex << std::uppercase << std::setfill('0') << std::setw(n_bytes * 2) << +v;
    return out.str();
}
