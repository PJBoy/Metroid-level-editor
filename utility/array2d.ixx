module;

#include "../global.h"

export module array2d;

export namespace array2d
{
struct Sizes
{
    n_t n_y, n_x;
};
}

using namespace array2d;

export template<typename T>
class Array2d
{
    std::vector<T> data1; // Would rather use `data`, but there needs to be a function named that
    Sizes sizes;

public:
    Array2d() = default;
    explicit Array2d(Sizes sizes);

    T* data();
    const T* data() const;
    n_t size() const;

    std::span<T> operator[](index_t i);
    std::span<const T> operator[](index_t i) const;
    Sizes getSizes() const;
};

template<typename T>
Array2d<T>::Array2d(Sizes sizes)
try
    : data1(sizes.n_y * sizes.n_x),
    sizes(std::move(sizes))
{}
LOG_RETHROW

template<typename T>
T* Array2d<T>::data()
{
    return std::data(data1);
}

template<typename T>
const T* Array2d<T>::data() const
{
    return std::data(data1);
}

template<typename T>
n_t Array2d<T>::size() const
{
    return std::size(data1);
}

template<typename T>
std::span<T> Array2d<T>::operator[](index_t i_y)
{
    return {&data1[i_y * sizes.n_x], sizes.n_x};
}

template<typename T>
std::span<const T> Array2d<T>::operator[](index_t i_y) const
{
    return {&data1[i_y * sizes.n_x], sizes.n_x};
}

template<typename T>
Sizes Array2d<T>::getSizes() const
{
    return sizes;
}

