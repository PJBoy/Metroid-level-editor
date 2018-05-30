#pragma once

#include <cstddef>
#include <string>
#include <type_traits>

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

template<typename T>
class Wrapper
{
protected:
    T v;

public:
    template<typename... Args>
    explicit Wrapper(Args&&... args)
        : v(std::forward<Args>(args)...)
    {}

    template<typename... Args>
    auto operator=(Args&&... args)
    {
        return v = T(std::forward<Args>(args)...);
    }

    template<typename U>
    explicit operator U() const
    {
        return U(v);
    }

#define operators(op) \
    template<typename U> \
    Wrapper& operator##op##=(const U& rhs) \
    { \
        v op##= rhs; \
        return *this; \
    } \
\
    Wrapper operator##op(const Wrapper& rhs) const \
    { \
        Wrapper t(*this); \
        return t op##= rhs; \
    } \
\
    template<typename U> \
    friend Wrapper operator##op(const Wrapper& lhs, const U& rhs) \
    { \
        Wrapper t(lhs); \
        return t op##= rhs; \
    } \
\
    template<typename U> \
    friend Wrapper operator##op(const U& lhs, const Wrapper& rhs) \
    { \
        return Wrapper(lhs) op rhs; \
    }

    operators(+)
    operators(-)
    operators(*)
    operators(/)
    operators(%)
    operators(|)
    operators(^)
    operators(&)
    operators(<<)
    operators(>>)
    operators(<)
    operators(>)
#undef operators

#define operator(op) \
\
    auto operator##op(const Wrapper& rhs) const \
    { \
        return v op rhs; \
    } \
\
    template<typename U> \
    friend auto operator##op(const Wrapper& lhs, const U& rhs) \
    { \
        return lhs.v op rhs; \
    } \
\
    template<typename U> \
    friend auto operator##op(const U& lhs, const Wrapper& rhs) \
    { \
        return lhs op rhs.v; \
    }

    operator(==)
    operator(!=)
    operator(&&)
    operator(||)
    operator(->*)
#undef operator

    auto operator,(const Wrapper<T>& rhs)
    {
        return v, rhs;
    }

    template<typename U>
    friend auto operator,(const Wrapper<T>& lhs, const U& rhs)
    {
        return lhs.v, rhs;
    }

    template<typename U>
    friend auto operator,(const U& lhs, const Wrapper<T>& rhs)
    {
        return lhs, rhs.v;
    }

#define operator(op) \
    auto operator##op() \
    { \
        return op v; \
    } \
\
    auto operator##op() const \
    { \
        return op v; \
    }

    operator(*)
    operator(&)
    operator(~)
    operator(!)
    operator(++)
    operator(--)
#undef operator

    auto operator->()
    {
        return &v;
    }

    auto operator++(int)
    {
        return v++;
    }

    auto operator--(int)
    {
        return v--;
    }

    template<typename... Args>
    auto operator()(Args... args)
    {
        return v(args...);
    }

    template<typename T>
    auto operator[](T i)
    {
        return v[i];
    }
};
