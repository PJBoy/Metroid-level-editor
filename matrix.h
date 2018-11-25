#pragma once

#include "global.h"

template<typename T>
class Matrix
{
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    n_t n_x{}, n_y{};
    std::vector<T> v;

public:
    Matrix() = default;

    Matrix(n_t n_y, n_t n_x, const T& value = {})
    try
        : n_y(n_y), n_x(n_x), v(n_y * n_x, value)
    {}
    LOG_RETHROW

    iterator operator[](index_t i_y) noexcept
    {
        return std::begin(v) + i_y * n_x;
    }

    const_iterator operator[](index_t i_y) const noexcept
    {
        return std::begin(v) + i_y * n_x;
    }

    iterator begin() noexcept
    {
        return std::begin(v);
    }

    const_iterator begin() const noexcept
    {
        return std::begin(v);
    }

    iterator end() noexcept
    {
        return std::end(v);
    }

    const_iterator end() const noexcept
    {
        return std::end(v);
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

    bool empty() const noexcept
    {
        return std::empty(v);
    }

    operator bool() const noexcept
    {
        return !empty();
    }
};
