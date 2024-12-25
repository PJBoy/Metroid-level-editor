export module string;

import std;

export std::string toString(std::wstring_view from) noexcept
{
    // TODO: wstring_convert et al are deprecated as of C++17
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(std::wstring(from));
}

export std::wstring toWstring(std::string_view from) noexcept
{
    // TODO: wstring_convert et al are deprecated as of C++17
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(std::string(from));
}
