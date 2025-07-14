#include "Manager/LogManager.h"
#include "Utils/TextUtils.h"

#include <process.h>

void LogManager::OnCreate()
{
    auto time = std::chrono::system_clock::now();
    auto time_str = TextUtils::ConvertTimeToString(time);
    int pid = _getpid();
    log_name_str = time_str + "-" + std::to_string(pid) + ".log";
    fs::path filePath = LogPath;
    fs::create_directories(filePath);
    _filePath = filePath / log_name_str;
    EnsureLogFile();
    _running = true;
    _writerThread = std::thread(&LogManager::WriterFunction, this);
}

void LogManager::OnDestroy()
{
    _running = false;
    _cv.notify_one();
    if (_writerThread.joinable()) {
        _writerThread.join();
    }
    _file.close();
}

void LogManager::WriterFunction()
{
    while (_running) {
        std::unique_lock<std::mutex> lock(_queueMutex);
        _cv.wait(lock, [this] { return !_messageQueue.empty() || !_running; });

        while (!_messageQueue.empty()) {
            EnsureLogFile();
            _file << _messageQueue.front() << '\n';
            if (_file.fail()) {
                throw std::runtime_error("写入日志文件失败");
            }
            _messageQueue.pop();
        }

        _file.flush();
    }
}

void LogManager::EnsureLogFile()
{
    if (!_file.is_open()) {
        _file.open(_filePath, std::ios::app);
    }
}

void LogManager::AddMessage(const std::string& message)
{
    std::lock_guard<std::mutex> lock(_queueMutex);
    _messageQueue.push(message);
    _cv.notify_one();
}