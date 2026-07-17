#include "TextCodec.h"

#include <windows.h>

std::string wideToUtf8(const std::wstring& value)
{
    if (value.empty()) {
        return "";
    }

    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return "";
    }

    std::string result(static_cast<size_t>(size), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), &result[0], size, nullptr, nullptr) <= 0) {
        return "";
    }

    return result;
}

std::wstring utf8ToWide(const std::string& value)
{
    if (value.empty()) {
        return L"";
    }

    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0) {
        return L"";
    }

    std::wstring result(static_cast<size_t>(size), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), &result[0], size) <= 0) {
        return L"";
    }

    return result;
}
