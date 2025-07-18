#pragma once

#include <chrono>
#include <string>
#include <sstream>

class LogUtils {
public:
	static void Log(std::string str);
	static void Error(std::string str);
};