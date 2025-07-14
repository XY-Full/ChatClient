#include "Manager/ApplicationInstance.h"
#include "Utils/TextUtils.h"

#include <process.h>

void ApplicationInstance::OnCreate()
{
    app_create_time = std::chrono::system_clock::now();
    create_time_str = TextUtils::ConvertTimeToString(app_create_time);
    pid = _getpid();
    log_name_str = create_time_str + "-" + std::to_string(pid) + ".log";
}
