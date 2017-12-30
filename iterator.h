#pragma once

#include "global.h"

#include <iterator>
#include <type_traits>

template<typename T>
class Wrapper
{
    T v;

public:
    template<typename... Args>
    Wrapper(Args... args)
        : v(args...)
    {}

    template<typename... Args>
    auto operator=(Args... args)
    {
        return v = T(args...);
    }

    template<typename U>
    operator U() const
    {
        return v;
    }

#define operator(op) \
    auto operator##op(const Wrapper<T>& rhs) \
    { \
        return v op rhs; \
    } \
\
    template<typename U> \
    friend auto operator##op(const Wrapper<T>& lhs, const U& rhs) \
    { \
        return lhs.v op rhs; \
    } \
\
    template<typename U> \
    friend auto operator##op(const U& lhs, const Wrapper<T>& rhs) \
    { \
        return lhs op rhs.v; \
    }

#define operators(op) \
    operator(op) \
    operator(op##=)

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

template<typename It>
class SentinelIterator : public Wrapper<It>
{
    using Wrapper = Wrapper<It>;

public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = Wrapper;
    using difference_type = std::ptrdiff_t;
    using pointer = Wrapper*;

    using reference = value_type&;

    using Wrapper::Wrapper;
    using Wrapper::operator*;
    using Wrapper::operator->;
    using Wrapper::operator++;

private:
    bool operator!() const
    {
        return static_cast<Wrapper>(*this) == Wrapper{};
    }

public:
    SentinelIterator() = default;

    SentinelIterator(It it)
        : Wrapper(it)
    {}

    bool operator==(const SentinelIterator& rhs) const
    {
        return !*this == !rhs || static_cast<Wrapper>(*this) == static_cast<Wrapper>(rhs);
    }

    bool operator!=(const SentinelIterator& rhs) const
    {
        return !(*this == rhs);
    }
};
