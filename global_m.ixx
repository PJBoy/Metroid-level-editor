export module global_m;

export import typedefs_m;

import std.core;

export template<typename T, std::enable_if_t<std::is_enum_v<T>>* = nullptr>
constexpr auto toInt(T v) noexcept
{
    return static_cast<std::underlying_type_t<T>>(v);
}

export template<typename T, typename Deleter>
constexpr std::unique_ptr<T, Deleter> makeUniquePtr(T* p, Deleter&& deleter)
{
    return std::unique_ptr<T, decltype(deleter)>(p, std::forward<Deleter>(deleter));
}

export template<typename T>
constexpr std::string toHexString(T v, n_t n_bytes = sizeof(T))
{
    std::ostringstream out;
    out << std::hex << std::uppercase << std::setfill('0') << std::setw(n_bytes * 2) << +v;
    return out.str();
}
