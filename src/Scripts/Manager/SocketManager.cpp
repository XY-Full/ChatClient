#include "Manager/SocketManager.h"
#include "Utils/LogUtils.h"

#include <iostream>
#include <locale>
#include <codecvt>

std::once_flag SocketManager::flag;

void SocketManager::TiggerMessageAction(MessageActionType type, std::string str)
{
    if (!MessageActionMap.empty())
    {
        auto it = MessageActionMap.find(type);
        if (it != MessageActionMap.end()) {
            // eval all function in vector
            for (const auto& func : it->second) {
                func(str);
            }
        }
        else {
            std::stringstream ss;
            ss << "Unknown command" << enumToString(type);
            LogUtils::Log("Unknown command MessageActionType::ReceiveMessage");
        }
    }
}

void SocketManager::SendAuthorizeMessage()
{
    std::stringstream ss;
    std::string str = "xyc";
    uint8_t* buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(str.data()));
    int size = static_cast<int>(str.size());
    ss << "authorize send: " << str;
    LogUtils::Log(ss.str());
    ss.str("");
    SendMessageData(buf, size);
}

bool SocketManager::ConnectServer(const char* server_ip, int server_port)
{
    std::stringstream ss;
    std::unique_lock<std::mutex> lock(socket_mutex);
    server_connecting = true;
    if (sock_cache == INVALID_SOCKET)
    {
#ifdef _Win
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            LogUtils::Error("WSAStartup fail");
            server_connecting = false;
            return false;
        }
#endif

        // 创建套接字
        sock_cache = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock_cache == INVALID_SOCKET) {
            LogUtils::Error("create socket failed");
#ifdef _Win
            WSACleanup();
#endif
            server_connecting = false;
            return false;
        }

    }
    else
    {
        server_connecting = false;
        LogUtils::Log("sock_cache valid");
        return true;
    }

#ifdef _WIN32
    // Windows 代码
    u_long mode = 0;  // 1 for non-blocking, 0 for blocking
    if (ioctlsocket(sock_cache, FIONBIO, &mode) != 0) {
        std::cerr << "Failed to set socket mode: " << WSAGetLastError() << std::endl;
        server_connecting = false;
        return false;
    }
#else
    // POSIX 代码
    int flags = fcntl(sock_cache, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Failed to get socket flags: " << strerror(errno) << std::endl;
        server_connecting = false;
        return false;
    }
    if (fcntl(sock_cache, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Failed to set non-blocking mode: " << strerror(errno) << std::endl;
        server_connecting = false;
        return false;
    }
#endif

    ServerIP = server_ip;
    ServerPort = server_port;

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &(server_addr.sin_addr));

    if (connect(sock_cache, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        ss << "socket connect failed addr: " << server_ip << " port: " << server_port;
        LogUtils::Log(ss.str());
        ss.str("");
        Disconnect();
        return false;
    }


    server_connected = true;
    SendAuthorizeMessage();

    if (!receive_thread.joinable())
    {
        receive_thread = std::thread([this]() {
            this->ReceiveMessage();
            });
    }

    if (!heart_thread.joinable())
    {
        heart_thread = std::thread([this]() {
            this->SendHeartThread();
            });
    }

    last_send_time = std::chrono::steady_clock::now();

    ss << "connect to server success: ip: " << server_ip << " port: " << server_port;
    LogUtils::Log(ss.str());
    TiggerMessageAction(MessageActionType::OnPrintMessage, ss.str());
    server_connecting = false;
    return true;
}

void SocketManager::Disconnect()
{
    std::unique_lock<std::mutex> lock(socket_mutex);
    server_connected = false;
    if (sock_cache != INVALID_SOCKET)
    {
        closesocket(sock_cache);
        sock_cache = INVALID_SOCKET;
    }
#ifdef _Win
    WSACleanup();
#endif
    LogUtils::Log("socket disconnected");
    TiggerMessageAction(MessageActionType::OnPrintMessage, "socket disconnected");
}

void SocketManager::ReceiveMessage()
{
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    std::stringstream ss;
    fd_set readfds;
    struct timeval tv;

    while (!should_stop) {
        FD_ZERO(&readfds);
        if (server_connecting) continue;
        if (sock_cache == INVALID_SOCKET)
        {

            Reconnect();
            continue;
        }

        FD_SET(sock_cache, &readfds);

        // 设置 select 超时为 100 毫秒
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int activity = select(sock_cache + 1, &readfds, NULL, NULL, &tv);

        if (activity == -1) {
            LogUtils::Error("Select error");
            continue;
        }

        if (activity == 0) {
            // 超时，继续下一次循环
            continue;
        }

        if (FD_ISSET(sock_cache, &readfds) && server_connected) {
            iResult = recv(sock_cache, recvbuf, DEFAULT_BUFLEN, 0);
            if (iResult > 0) {
                std::string str = "";
                try {
                    str = std::string(recvbuf, iResult);
                }
                catch (const std::exception& e) {
                    ss << "Failed to convert to UTF-8: " << e.what();
                    LogUtils::Error(ss.str());
                    ss.str("");
                    continue;
                }
                if (str == "PONG")
                {
                    heart_received = true;
                    continue;
                }
                TiggerMessageAction(MessageActionType::ReceiveMessage, str);
                ss << "yeyeye: " << str;
                LogUtils::Log(ss.str());
                ss.str("");
            }
            else if (iResult == 0) {
                LogUtils::Log("Connection closed by server");
                Disconnect();
                Reconnect();
            }
            else {
                // 检查是否是"无数据可读"的情况
#ifdef _WIN32
                if (WSAGetLastError() == WSAEWOULDBLOCK) {
                    // 没有数据可读，这是正常的，继续下一次循环
                    continue;
                }
                ss << "recv failed: " << WSAGetLastError();
#else
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 没有数据可读，这是正常的，继续下一次循环
                    continue;
                }
                ss << "recv failed: " << strerror(errno);
#endif
                Disconnect();
                Reconnect();
                LogUtils::Log(ss.str());
                ss.str("");
            }
        }
    }
}


void SocketManager::SendHeartThread()
{
    std::string heart_str = "PING";
    uint8_t* buf;
    int size;
    std::stringstream ss;
    last_send_time = std::chrono::steady_clock::now();
    heart_received = true;
    while (!should_stop)
    {
        if (sock_cache == INVALID_SOCKET && !server_connected) continue;
        if (server_connecting) continue;

        // 获取当前时间
        auto now = std::chrono::steady_clock::now();

        // 计算距离上次处理的时间
        auto send_duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_send_time);

        // 如果时间间隔大于等于 10 秒
        if (send_duration.count() >= HEART_SECOND) {
            if (!heart_received) {
                ss << "heart pak time out :" << HEART_SECOND << "s";
                LogUtils::Error(ss.str());
                ss.str("");
                Disconnect();
                continue;
            }
            buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(heart_str.data()));
            size = static_cast<int>(heart_str.size());
            SendMessageData(buf, size);
            last_send_time = now;  // 更新上次处理时间
            heart_received = false;
        }

        // 短暂休眠以减少 CPU 使用
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void SocketManager::Reconnect()
{
    std::stringstream ss;
    ss << "try to reconnect ip: " << ServerIP << " port: " << ServerPort;
    LogUtils::Log(ss.str());
    ss.str("");

    bool connected = ConnectServer(ServerIP, ServerPort);

    ss << "reconnect result: " << connected;
    LogUtils::Log(ss.str());

    if (!connected)
    {
        Disconnect();
    }
}

bool SocketManager::SendMessageData(uint8_t* buf, int size)
{
    if (sock_cache == INVALID_SOCKET) 
    {
        LogUtils::Error("socket invalid, need create socket first");
        return false;
    }
    char* message = reinterpret_cast<char*>(buf);
    // 发送数据
    std::stringstream ss;
    ss << "try to send message: " << std::string(message);
    LogUtils::Log(ss.str());
    if (send(sock_cache, message, size, 0) == SOCKET_ERROR) {
        LogUtils::Error("send message fail: " + std::string(message));
        return false;
    }
    return true;
}

void SocketManager::RegisterMessageEvent(MessageActionType type, FunctionInString func)
{
    MessageActionMap[type].push_back(func);
}