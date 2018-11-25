#pragma once

#include "rom.h"

#include "global.h"

#include <cinttypes>

class Gba : public Rom
{
public:
    using byte_t     = std::uint8_t;
    using halfword_t = std::uint16_t;
    using word_t     = std::uint32_t;


protected:
    struct Pointer
    {
        word_t v{};

    #define operators(op) \
        constexpr Pointer operator##op(Pointer rhs) \
        { \
            return {v op word_t(rhs.v)}; \
        } \
    \
        template<typename U> \
        constexpr friend Pointer operator##op(Pointer lhs, U rhs) \
        { \
            return {lhs.v op word_t(rhs)}; \
        } \
    \
        template<typename U> \
        constexpr friend Pointer operator##op(U lhs, Pointer rhs) \
        { \
            return {word_t(lhs) op rhs.v}; \
        } \
    \
        template<typename U> \
        constexpr Pointer& operator##op##=(U rhs) \
        { \
            return v op##= rhs, *this; \
        } \

        operators(+)
        operators(-)
        operators(|)
        operators(&)
    #undef operators

    #define operator(op) \
        constexpr bool operator##op(Pointer rhs) \
        { \
            return v op rhs.v; \
        } \

        operator(<)
        operator(>)
        operator(<=)
        operator(>=)
        operator(==)
        operator(!=)
    #undef operator

        constexpr Pointer& operator++()
        {
            return ++v, *this;
        }

        constexpr Pointer& operator--()
        {
            return --v, *this;
        }

        constexpr Pointer operator++(int)
        {
            return ++*this - 1;
        }

        constexpr Pointer operator--(int)
        {
            return --*this + 1;
        }
    };

    explicit Gba(std::filesystem::path filepath);

    constexpr friend Pointer operator"" _gba(unsigned long long pointer);
};

constexpr Gba::Pointer operator"" _gba(unsigned long long pointer)
{
    return {Gba::word_t(pointer)};
}