#include "SocketManager.h"

#include <iostream>
#include <locale>
#include <codecvt>

std::once_flag SocketManager::flag;

bool SocketManager::ConnectServer(const char* server_ip, int server_port)
{
    if (sock_cache == INVALID_SOCKET)
    {
#ifdef _Win
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup fail" << std::endl;
            return false;
        }
#endif

        // 创建套接字
        sock_cache = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock_cache == INVALID_SOCKET) {
            std::cerr << "create socket failed" << std::endl;
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
        std::cerr << "socket connect failed addr: " << server_ip << " port: " << server_port << std::endl;
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

    std::clog << "connect to server success: ip: " << server_ip << " port: " << server_port << std::endl;

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
    std::clog << "socket disconnected" << std::endl;
    return;
}

void SocketManager::ReceiveMessage()
{
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;

    while (sock_cache != INVALID_SOCKET) {
        iResult = recv(sock_cache, recvbuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0) {
            //recvbuf[iResult] = '\0';
            //std::cout << "Bytes received: " << iResult << std::endl;
            std::string str = "";
            try {
                str = std::string(recvbuf, iResult);
                //std::cout << "Received data (UTF-8): " + str << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "Failed to convert to UTF-8: " << e.what() << std::endl;
                continue;
            }
            if (str == "PONG")
            {
                last_receive_time = std::chrono::steady_clock::now();
                continue;
            }
            if (!MessageActionMap.empty())
            {
                auto it = MessageActionMap.find(MessageActionType::ReceiveMessage);
                if (it != MessageActionMap.end()) {
                    // eval all function in vector
                    for (const auto& func : it->second) {
                        func(str);
                    }
                }
                else {
                    std::cout << "Unknown command MessageActionType::ReceiveMessage" << std::endl;
                }
            }
            std::cout << "yeyeye: " << str << std::endl;
        }
        else if (iResult == 0) {
            std::cout << "Connection closed by server" << std::endl;
        }
        else {
#ifdef _WIN32
            std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
#else
            std::cerr << "recv failed: " << strerror(errno) << std::endl;
#endif
        }
    }
}

void SocketManager::SendHeartThread()
{
    std::string str = "xyc";
    uint8_t* buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(str.data()));
    int size = static_cast<int>(str.size());
    std::clog << "heart send: " << str << std::endl;
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
            //std::clog << "heart send: " << str << std::endl;
            SendMessageData(buf, size);
            lastProcessTime = now;  // 更新上次处理时间
        }

        if (receive_duration.count() >= HEART_SECOND * 1.5)
        {
            std::cerr << "heart pak time out :" << HEART_SECOND * 1.5 << "s" << std::endl;
            Disconnect();
            Reconnect();
        }

        // 短暂休眠以减少 CPU 使用
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void SocketManager::Reconnect()
{
    std::clog << "try to reconnect ip: " << ServerIP << " port: " << ServerPort << std::endl;
    bool connected = ConnectServer(ServerIP, ServerPort);
    std::clog << "reconnect result: " << connected << std::endl;
    if (!connected)
    {
        Disconnect();
    }
}

bool SocketManager::SendMessageData(uint8_t* buf, int size)
{
    if (sock_cache == INVALID_SOCKET)
    {
        std::cerr << "socket invalid, need create socket first" << std::endl;
        return false;
    }
    char* message = reinterpret_cast<char*>(buf);
    // 发送数据
    if (send(sock_cache, message, size, 0) == SOCKET_ERROR) {
        std::cerr << "send message fail: " << message << std::endl;
        return false;
    }
    return true;
}

void SocketManager::RegisterMessageEvent(MessageActionType type, FunctionInString func)
{
    MessageActionMap[type].push_back(func);
}