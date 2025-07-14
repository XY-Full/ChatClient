#pragma once

#include <iostream>
#include <vector>
#include <chrono>

class TextUtils {
public:
	static std::vector<char> wstring_to_utf8(const std::wstring& wstr);
	static void string_to_buffer(std::string& str, uint8_t*& buf, int& size);
	static void wstring_to_buffer(const std::wstring& wstr, uint8_t*& buf, int& size);
	static bool string_to_wchar_buffer(const std::string& str, wchar_t buffer[], size_t buffer_size);
	static std::wstring string_to_wstring(const std::string& str);
	static std::string ConvertTimeToString(std::chrono::system_clock::time_point time_ptr);
};