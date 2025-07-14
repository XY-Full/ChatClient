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

bool SocketManager::ConnectServer(const char* server_ip, int server_port)
{
    std::stringstream ss;
    if (sock_cache == INVALID_SOCKET)
    {
#ifdef _Win
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            LogUtils::Error("WSAStartup fail");
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
            return false;
        }

    }

    ServerIP = server_ip;
    ServerPort = server_port;

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip, &(server_addr.sin_addr));

    if (connect(sock_cache, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        ss << "socket connect failed addr: " << server_ip << " port: " << server_port;
        LogUtils::Log(ss.str());
        ss.clear();
        Disconnect();
        return false;
    }

    receive_thread = std::thread([this]() {
        this->ReceiveMessage();
        });
    heart_thread = std::thread([this]() {
        this->SendHeartThread();
        });

    last_receive_time = std::chrono::steady_clock::now();

    ss << "connect to server success: ip: " << server_ip << " port: " << server_port;
    LogUtils::Log(ss.str());

    return true;
}

void SocketManager::Disconnect()
{
#ifdef _Win
    WSACleanup();
#endif
    if (receive_thread.joinable()) {
        receive_thread.join();
    }
    if (heart_thread.joinable()) {
        heart_thread.join();
    }
    if (sock_cache == INVALID_SOCKET)
    {
        return;
    }
    closesocket(sock_cache);
    sock_cache = INVALID_SOCKET;
    LogUtils::Log("socket disconnected");
    TiggerMessageAction(MessageActionType::Disconnected, "");
    return;
}

void SocketManager::ReceiveMessage()
{
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;

    std::stringstream ss;

    while (sock_cache != INVALID_SOCKET) {
        iResult = recv(sock_cache, recvbuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0) {
            std::string str = "";
            try {
                str = std::string(recvbuf, iResult);
            }
            catch (const std::exception& e) {
                ss << "Failed to convert to UTF-8: " << e.what();
                LogUtils::Error(ss.str());
                ss.clear();
                continue;
            }
            if (str == "PONG")
            {
                last_receive_time = std::chrono::steady_clock::now();
                continue;
            }
            TiggerMessageAction(MessageActionType::ReceiveMessage, str);
            ss << "yeyeye: " << str;
            LogUtils::Log(ss.str());
            ss.clear();
        }
        else if (iResult == 0) {
            LogUtils::Log("Connection closed by server");
        }
        else {
#ifdef _WIN32
            ss << "recv failed: " << WSAGetLastError();
#else
            ss << "recv failed: " << strerror(errno);
#endif
            LogUtils::Log(ss.str());
            ss.clear();
            Disconnect();
        }
    }
}

void SocketManager::SendHeartThread()
{
    std::stringstream ss;
    std::string str = "xyc";
    uint8_t* buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(str.data()));
    int size = static_cast<int>(str.size());
    ss << "authorize send: " << str;
    LogUtils::Log(ss.str());
    ss.clear();
    SendMessageData(buf, size);

    auto lastProcessTime = std::chrono::steady_clock::now();
    while (sock_cache != INVALID_SOCKET)
    {
        // 获取当前时间
        auto now = std::chrono::steady_clock::now();

        // 计算距离上次处理的时间
        auto send_duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastProcessTime);
        auto receive_duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_receive_time);

        // 如果时间间隔大于等于 10 秒
        if (send_duration.count() >= HEART_SECOND) {
            str = "PING";
            buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(str.data()));
            size = static_cast<int>(str.size());
            SendMessageData(buf, size);
            lastProcessTime = now;  // 更新上次处理时间
        }

        if (receive_duration.count() >= HEART_SECOND * 1.5)
        {
            ss << "heart pak time out :" << HEART_SECOND * 1.5 << "s";
            LogUtils::Error(ss.str());
            ss.clear();
            Disconnect();
            Reconnect();
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
    ss.clear();

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