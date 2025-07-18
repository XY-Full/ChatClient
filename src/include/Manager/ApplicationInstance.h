#pragma once

#include <chrono>
#include <string>

#include "Common/Singleton.h"

class ApplicationInstance : public Singleton<ApplicationInstance> {
    friend class Singleton<ApplicationInstance>;
private:

    std::chrono::system_clock::time_point app_create_time;
    std::string create_time_str;
    std::string log_name_str;
    int pid;

    void OnCreate() override;

public:
    std::chrono::system_clock::time_point inline GetCreateTime() { return app_create_time; };
    std::string inline GetCreateTimeStr() { return create_time_str; };
    std::string inline GetLogNameStr() { return log_name_str; };
    int inline GetPID() { return pid; };
};