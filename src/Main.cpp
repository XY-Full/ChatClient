#include <iostream>
#include <string>
#include <limits>
#include <io.h>
#include <fcntl.h>

#include "SocketManager.h"
#include "Config.h"

#undef max

void processInput(const std::string& input) {
    // ������ʵ������߼�
    //std::cout << "���������: " << input << std::endl;
    // ���Ը�����������ִ�в�ͬ�Ĳ���
    if (input == "exit") {
        std::cout << "�˳�����" << std::endl;
        exit(0);
    }
    // ��Ӹ�����߼�...
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

    // ���ÿ���̨����ҳΪ UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // ���� cin �� cout �� UTF-8 ģʽ����
    //_setmode(_fileno(stdin), _O_U8TEXT);
    //_setmode(_fileno(stdout), _O_U8TEXT);

    const char* server_ip = "156.225.20.138";
    int server_port = 6670;
    if (!manager->ConnectServer(server_ip, server_port))
    {
        return 0;
    }

    while (true) {
        // ��ȡһ��������
        //std::wstring input;
        //std::getline(std::wcin, input);

        HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
        WCHAR buffer[1024];
        DWORD read;

        ReadConsoleW(hStdin, buffer, 1024, &read, NULL);

        std::wstring input(buffer, read - 2);

        // �����ַ���ת��Ϊ UTF-8
        //int size_needed = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, NULL, 0, NULL, NULL);
        //std::string utf8_input(size_needed, 0);
        //WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, &utf8_input[0], size_needed, NULL, NULL);

        // ת��Ϊ UTF-8 ����� char* �ַ���
        std::vector<char> utf8_str = wstring_to_utf8(input);
        char* char_ptr = utf8_str.data();


        // ����Ƿ�ɹ���ȡ����
        if (std::cin.fail()) {
            std::cin.clear(); // ��������־
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "������������ԡ�" << std::endl;
            continue;
        }

        uint8_t* buf;
        int size;
        // ��������
        std::string str(utf8_str.begin(), utf8_str.end());
        //std::cout << str << std::endl;
        string_to_buffer(str, buf, size);
        manager->SendMessageData(buf, size);
    }

    return 0;
}
#endif