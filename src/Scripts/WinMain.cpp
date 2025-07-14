
#include "Manager/SocketManager.h"
#include <windows.h>
#include <string>
#include <Richedit.h>

#include "Common/Config.h"
#include "Utils/TextUtils.h"

const int ID_EDIT = 101;
const int ID_OUTPUT = 102;

HWND hEdit, hOutput;
HFONT hFont = NULL;  // 全局变量，用于存储字体句柄
std::vector<std::wstring> messages;

WNDPROC oldEditProc;

void UpdateOutput() {
    std::wstring allMessages;
    for (const auto& msg : messages) {
        allMessages += msg + L"\r\n";
    }
    SetWindowText(hOutput, allMessages.c_str());
    SendMessage(hOutput, EM_LINESCROLL, 0, SendMessage(hOutput, EM_GETLINECOUNT, 0, 0));
}

void ProcessInput() {
    wchar_t buffer[1024];
    GetWindowText(hEdit, buffer, 1024);
    std::wstring input(buffer);

    if (!input.empty()) {
        messages.push_back(L"xyc: " + input);
        UpdateOutput();

        uint8_t* buf;
        int size;
        std::vector<char> utf8_str = TextUtils::wstring_to_utf8(input);
        char* char_ptr = utf8_str.data();
        std::string str(utf8_str.begin(), utf8_str.end());
        buf = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(str.data()));
        size = static_cast<int>(str.size());
        //TextUtils::wstring_to_buffer(input, buf, size);
        SocketManager::GetInstance().SendMessageData(buf, size);
        // 清空输入框
        SetWindowText(hEdit, L"");
    }
}

void OnReceiveMessage(std::string str)
{
    std::wstring wstr = TextUtils::string_to_wstring(str);
    messages.push_back(L"yeyeye: " + wstr);
    UpdateOutput();
}

LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CHAR && wParam == VK_RETURN) {
        ProcessInput();
        return 0;
    }
    return CallWindowProc(oldEditProc, hWnd, uMsg, wParam, lParam);
}

void OnWindowClose(HWND hwnd)
{
    ExitProcess(0);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CLOSE:
    {
        OnWindowClose(hwnd);
        return 0;
        break;
    }
    case WM_CREATE:
    {
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Consolas");

        hOutput = CreateWindowEx(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
            10, 10, rcClient.right - 20, rcClient.bottom - 70,
            hwnd, (HMENU)ID_OUTPUT, NULL, NULL);

        if (hOutput)
        {
            // 设置背景色为黑色
            SendMessage(hOutput, EM_SETBKGNDCOLOR, 0, RGB(0, 0, 0));

            // 设置文本颜色为白色
            CHARFORMAT cf = { 0 };
            cf.cbSize = sizeof(CHARFORMAT);
            cf.dwMask = CFM_COLOR;
            cf.crTextColor = RGB(255, 255, 255);
            SendMessage(hOutput, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);

            // 设置字体
            SendMessage(hOutput, WM_SETFONT, (WPARAM)hFont, TRUE);
        }

        hEdit = CreateWindowEx(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            10, rcClient.bottom - 50, rcClient.right - 20, 40,
            hwnd, (HMENU)ID_EDIT, NULL, NULL);

        if (hEdit)
        {
            // 设置背景色为黑色
            SendMessage(hEdit, EM_SETBKGNDCOLOR, 0, RGB(0, 0, 0));

            // 设置文本颜色为白色
            CHARFORMAT cf = { 0 };
            cf.cbSize = sizeof(CHARFORMAT);
            cf.dwMask = CFM_COLOR;
            cf.crTextColor = RGB(255, 255, 255);
            SendMessage(hEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);

            // 设置字体
            SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        }

        // 子类化输入框以处理回车键
        oldEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
    }
    break;

    case WM_SIZE:
    {
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        SetWindowPos(hOutput, NULL, 10, 10, rcClient.right - 20, rcClient.bottom - 70, SWP_NOZORDER);
        SetWindowPos(hEdit, NULL, 10, rcClient.bottom - 50, rcClient.right - 20, 40, SWP_NOZORDER);
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
        EndPaint(hwnd, &ps);
    }
    return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

#if UseWinMain
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    const char* server_ip = "156.225.20.138";
    int server_port = 6670;
    if (!SocketManager::GetInstance().ConnectServer(server_ip, server_port))
    {
        return 0;
    }
    SocketManager::GetInstance().RegisterMessageEvent(MessageActionType::ReceiveMessage, OnReceiveMessage);
    
    const wchar_t CLASS_NAME[] = L"/xieyuchao01";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Chat Window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
#endif