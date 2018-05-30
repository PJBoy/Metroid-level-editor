#pragma once

#include "global.h"

#include <iterator>
#include <type_traits>

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
