#include "Utils/LogUtils.h"
#include "Manager/LogManager.h"
#include "Utils/TextUtils.h"

#include <chrono>

void LogUtils::Log(std::string str)
{
	LogManager& log_manager = LogManager().GetInstance();
	auto time = std::chrono::system_clock::now();
	auto time_str = TextUtils::ConvertTimeToString(time);
	std::stringstream ss;
	ss << "[" << time_str << "]" << str;
	log_manager.AddMessage(ss.str());
}

void LogUtils::Error(std::string str)
{
	LogManager& log_manager = LogManager().GetInstance();
	auto time = std::chrono::system_clock::now();
	auto time_str = TextUtils::ConvertTimeToString(time);
	std::stringstream ss;
	ss << "[" << time_str << "]" << "Error: " << str;
	log_manager.AddMessage(ss.str());
}