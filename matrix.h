#pragma once

#include "global.h"

#include <compare>
#include <iterator>
#include <vector>

template <typename T>
class Matrix
{
public:
    // Container requires that these are ForwardIterators
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    // ReversibleContainer requires these typedefs exist and are defined as below
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Container requires that these typedefs exist and are defined as below
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;

    using difference_type = typename std::vector<T>::difference_type;
    using size_type = typename std::vector<T>::size_type;

private:
    n_t n_x{}, n_y{};
    std::vector<T> v;

public:
    // Container requires this exists such that empty() = true
    Matrix() = default;

    // Container requires that copy/move constructors/assignment operators exist and satisfy obvious equality post-conditions
    // Container requires that the assignment operators return Container&
    // Container requires that the destructor destroys every element and frees all memory
    // The implicitly generated functions satisfy these requirements

    Matrix(n_t n_y, n_t n_x, const T& value = {})
    try
        : n_y(n_y), n_x(n_x), v(n_y * n_x, value)
    {}
    LOG_RETHROW

    // Container requires (in)equality operators to exist and be defined as below
    bool operator==(const Matrix& rhs) const noexcept
    {
        return std::equal(begin(), end(), rhs.begin(), rhs.end());
    }

    bool operator!=(const Matrix& rhs) const noexcept
    {
        return !(*this == rhs);
    }

    iterator operator[](index_t i_y) noexcept
    {
        return std::begin(v) + i_y * n_x;
    }

    const_iterator operator[](index_t i_y) const noexcept
    {
        return std::begin(v) + i_y * n_x;
    }

    operator bool() const noexcept
    {
        return !empty();
    }

    // Container requires (c)begin/end exist and return (const_)iterator
    iterator begin() noexcept
    {
        return std::begin(v);
    }

    const_iterator begin() const noexcept
    {
        return std::begin(v);
    }

    // Container requires cbegin/cend exist and are defined as below
    const_iterator cbegin() const noexcept
    {
        return begin();
    }

    iterator end() noexcept
    {
        return std::end(v);
    }

    const_iterator end() const noexcept
    {
        return std::end(v);
    }

    const_iterator cend() const noexcept
    {
        return end();
    }

    // ReversibleContainer requires (c)rbegin/end functions defined as below
    reverse_iterator rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    const_reverse_iterator rbegin() const noexcept
    {
        return reverse_iterator(end());
    }

    const_reverse_iterator crbegin() const noexcept
    {
        return rbegin();
    }

    reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rend() const noexcept
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator crend() const noexcept
    {
        return rend();
    }

    // Container requires this to exist and to not invoke any move, copy or swap operations on the contained elements
    void swap(Matrix& rhs) const noexcept
    {
        std::swap(*this, rhs);
    }

    // Container requires this to exist and is defined as below
    friend void swap(Matrix& lhs, Matrix& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    // Container requires that empty, size and max_size exist and are defined as below
    bool empty() const noexcept
    {
        return begin() == end();
    }

    size_type max_size() const noexcept
    {
        return v.max_size();
    }

    size_type size() const noexcept
    {
        return std::distance(begin(), end());
    }

    n_t size_x() const noexcept
    {
        return n_x;
    }

    n_t size_y() const noexcept
    {
        return n_y;
    }

    T* data() noexcept
    {
        return std::data(v);
    }

    const T* data() const noexcept
    {
        return std::data(v);
    }
};
