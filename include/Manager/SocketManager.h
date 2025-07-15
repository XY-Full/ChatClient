#pragma once

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

#include <vector>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <chrono>
#include <map>
#include <functional>

#include "Common/Singleton.h"
#include "Const/ConstMessage.h"

#define DEFAULT_BUFLEN 4096
#define HEART_SECOND 5

#define _Win defined(_WIN32) || defined(_WIN64)

using FunctionInString = std::function<void(std::string)>;

class SocketManager : public Singleton<SocketManager>{
    friend class Singleton<SocketManager>;
private:
    static std::once_flag flag;

    const char* ServerIP;
    int ServerPort;

    std::condition_variable cv;
    std::atomic<bool> should_stop{ false };
    std::atomic<bool> server_connected{ false };
    std::atomic<bool> server_connecting{ false };
    std::atomic<bool> heart_received{ false };
    std::mutex socket_mutex;

    SOCKET sock_cache = INVALID_SOCKET;
    std::thread receive_thread;
    std::thread heart_thread;
    std::string decrypt_key = "";

    std::map<MessageActionType, std::vector<FunctionInString>> MessageActionMap;

    std::chrono::steady_clock::time_point last_send_time;

    SocketManager() {
        ServerIP = "0";
        ServerPort = 0;
    }

    void OnDestroy() override {
        Disconnect();
    }

    void TiggerMessageAction(MessageActionType type, std::string str);

    void SendAuthorizeMessage();

public:

    inline std::string GetDecryptKey() { return decrypt_key; }

    bool ConnectServer(const char* server_ip, int server_port);

    void Disconnect();

    bool SendMessageData(uint8_t* buf, int size);

    void ReceiveMessage();

    void SendHeartThread();

    void Reconnect();

    void RegisterMessageEvent(MessageActionType type, FunctionInString func);

};