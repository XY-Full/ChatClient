#include <iostream>
#include <string>
#include <limits>
#include <io.h>
#include <fcntl.h>

#include "SocketManager.h"
#include "Config.h"

#undef max

void processInput(const std::string& input) {
    // 在这里实现你的逻辑
    //std::cout << "你输入的是: " << input << std::endl;
    // 可以根据输入内容执行不同的操作
    if (input == "exit") {
        std::cout << "退出程序" << std::endl;
        exit(0);
    }
    // 添加更多的逻辑...
}

std::vector<char> wstring_to_utf8(const std::wstring& wstr)
{
    if (wstr.empty()) {
        return std::vector<char>(1, '\0');
    }

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::vector<char> utf8_str(size_needed + 1); // +1 for null terminator

    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &utf8_str[0], size_needed, NULL, NULL);
    utf8_str[size_needed] = '\0'; // Ensure null termination

    return utf8_str;
}

void string_to_buffer(std::string& str, uint8_t*& buf, int& size)
{
    buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(str.data()));
    size = static_cast<int>(str.size());
}

void wstring_to_buffer(const std::wstring& wstr, uint8_t*& buf, int& size)
{
    std::vector<char> utf8_str = wstring_to_utf8(wstr);
    char* char_ptr = utf8_str.data();
    std::string str(utf8_str.begin(), utf8_str.end());
    string_to_buffer(str, buf, size);
}

#if !UseWinMain
int main() {
    std::string input;
    SocketManager* manager = SocketManager::GetInstance();

    // 设置控制台代码页为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // 设置 cin 和 cout 以 UTF-8 模式工作
    //_setmode(_fileno(stdin), _O_U8TEXT);
    //_setmode(_fileno(stdout), _O_U8TEXT);

    const char* server_ip = "156.225.20.138";
    int server_port = 6670;
    if (!manager->ConnectServer(server_ip, server_port))
    {
        return 0;
    }

    while (true) {
        // 读取一整行输入
        //std::wstring input;
        //std::getline(std::wcin, input);

        HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
        WCHAR buffer[1024];
        DWORD read;

        ReadConsoleW(hStdin, buffer, 1024, &read, NULL);

        std::wstring input(buffer, read - 2);

        // 将宽字符串转换为 UTF-8
        //int size_needed = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, NULL, 0, NULL, NULL);
        //std::string utf8_input(size_needed, 0);
        //WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, &utf8_input[0], size_needed, NULL, NULL);

        // 转换为 UTF-8 编码的 char* 字符串
        std::vector<char> utf8_str = wstring_to_utf8(input);
        char* char_ptr = utf8_str.data();


        // 检查是否成功读取输入
        if (std::cin.fail()) {
            std::cin.clear(); // 清除错误标志
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "输入错误，请重试。" << std::endl;
            continue;
        }

        uint8_t* buf;
        int size;
        // 处理输入
        std::string str(utf8_str.begin(), utf8_str.end());
        //std::cout << str << std::endl;
        string_to_buffer(str, buf, size);
        manager->SendMessageData(buf, size);
    }

    return 0;
}
#endif