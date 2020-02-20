#pragma once

#include "global.h"

#include <cstring>

// Iterator that takes a byte array and interprets it as an array of integers (to be changed to std::bit_cast)
template<typename Int>
class ByteCastIterator
{
    static_assert(std::is_integral_v<Int>);

protected:
    std::byte* p{};
    Int v;

public:
    // Iterator requires these typedefs exist (or iterator_traits is specialised equivalently)
    using iterator_category = std::forward_iterator_tag;
    using value_type = Int;
    using difference_type = std::ptrdiff_t;
    using pointer = Int*;

    // InputIterator requires this is convertible to value_type
    // ForwardIterator requires that this is precisely value_type& or const value_type&
    using reference = value_type&;

    // ForwardIterator requires a default constructor
    ByteCastIterator() = default;

    explicit ByteCastIterator(void* p) noexcept
        : p(static_cast<std::byte*>(p))
    {
        if (p)
            ++*this;
    }

    // Iterator requires this exists
    // InputIterator requires this returns reference
    // InputIterator requires it_a == it_b implies *it_a is equivalent to *it_b
    // InputIterator requires (void)*it, *it is equivalent to *it
    // OutputIterator requires this returns an assignable type
    reference operator*() noexcept
    {
        return v;
    }

    const reference operator*() const noexcept
    {
        return v;
    }

    // InputIterator requires this exists and it->m is equivalent to (*it).m
    value_type* operator->() noexcept
    {
        return &**this;
    }

    const value_type* operator->() const noexcept
    {
        return &**this;
    }

    // Iterator requires this exists and to return an Iterator&
    // ForwardIterator requires that (void)++Iterator(it), *it is equivalent to *it
    ByteCastIterator& operator++() noexcept
    {
        std::memcpy(&v, p, sizeof(Int));
        p += sizeof(Int);

        return *this;
    }

    // InputIterator and OutputIterator require this exists
    // InputIterator requires this to return a type whose operator* returns a type convertible to value_type
    // InputIterator and OutputIterator require this is equivalent to the following
    // OutputIterator requires this to return a type convertible to const Iterator&
    // OutputIterator requires this to return an assignable type such that *it++ = v is equivalent to *r = v; ++it;
    // ForwardIterator requires this to return an Iterator
    // ForwardIterator requires this to return a type whose operator* returns reference
    ByteCastIterator operator++(int) noexcept
    {
        return std::exchange(*this, ++*this);
    }

    // Iterator requires this exists, is an equivalence relation and returns a type contextually convertible to bool
    // ForwardIterator requires that it_a == it_b implies *it_a and *it_b refer to the same object or are both not dereferenceable
    // ForwardIterator requires that it_a == it_b implies ++it_a == ++it_b
    bool operator==(const ByteCastIterator& rhs) const noexcept
    {
        return p == rhs.p;
    }

    // InputIterator requires this exists and is equivalent to the following
    bool operator!=(const ByteCastIterator& rhs) const noexcept
    {
        return !(*this == rhs);
    }
};
