#pragma once

#include <chrono>
#include <string>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <thread>
#include <queue>

#include "Common/Config.h"
#include "Common/Singleton.h"

namespace fs = std::filesystem;

class LogManager : public Singleton<LogManager> {
    friend class Singleton<LogManager>;
private:
    std::string log_name_str;
    fs::path _filePath;
    std::ofstream _file;

    std::mutex _queueMutex;
    std::queue<std::string> _messageQueue;
    std::condition_variable _cv;
    std::thread _writerThread;
    std::atomic<bool> _running;

    void OnCreate() override;
    void OnDestroy() override;

    void WriterFunction();
    void EnsureLogFile();

public:
    void AddMessage(const std::string&);
};