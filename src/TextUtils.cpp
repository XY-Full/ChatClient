#include "TextUtils.h"

#include <WinSock2.h>

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

void TextUtils::string_to_buffer(std::string& str, uint8_t*& buf, int& size)
{
    buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(str.data()));
    size = static_cast<int>(str.size());
}

void TextUtils::wstring_to_buffer(const std::wstring& wstr, uint8_t*& buf, int& size)
{
    std::vector<char> utf8_str = wstring_to_utf8(wstr);
    char* char_ptr = utf8_str.data();
    std::string str(utf8_str.begin(), utf8_str.end());
    buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(str.data()));
    size = static_cast<int>(str.size());
}

bool TextUtils::string_to_wchar_buffer(const std::string& str, wchar_t buffer[], size_t buffer_size) {
    // 获取需要的宽字符数（不包括结尾的 null 字符）
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);

    // 检查缓冲区大小是否足够
    if (size_needed > static_cast<int>(buffer_size)) {
        std::cerr << "Buffer too small. Required size: " << size_needed << std::endl;
        return false;
    }

    // 执行转换
    int result = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer, buffer_size);

    if (result <= 0) {
        std::cerr << "Conversion failed. Error code: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}

std::wstring TextUtils::string_to_wstring(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}