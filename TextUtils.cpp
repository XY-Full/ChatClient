#include "TextUtils.h"

#include <stringapiset.h>

std::vector<char> TextUtils::wstring_to_utf8(const std::wstring& wstr)
{
    if (wstr.empty()) {
        return std::vector<char>(1, '\0');
    }

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::vector<char> utf8_str(size_needed + 1); // +1 for null terminator

    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &utf8_str[0], size_needed, NULL, NULL);
    utf8_str[size_needed] = '\0'; // Ensure null termination

    return utf8_str;
}

void TextUtils::string_to_buffer(std::string& str, uint8_t *& buf, int& size)
{
    buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(str.data()));
    size = static_cast<int>(str.size());
}

void TextUtils::wstring_to_buffer(const std::wstring& wstr, uint8_t *& buf, int& size)
{
    std::vector<char> utf8_str = wstring_to_utf8(wstr);
    char* char_ptr = utf8_str.data();
    std::string str(utf8_str.begin(), utf8_str.end());
    string_to_buffer(str, buf, size);
}
