export module string;

import std.core;

export std::string toString(const std::wstring& from) noexcept
{
    // TODO: wstring_convert et al are deprecated as of C++17
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(from);
}

export std::wstring toWstring(const std::string& from) noexcept
{
    // TODO: wstring_convert et al are deprecated as of C++17
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(from);
}
