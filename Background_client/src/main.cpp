#define SECURITY_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <commctrl.h>

#include <sddl.h>
#include <Sspi.h>
#include <iphlpapi.h>
#include <comdef.h>
#include <wincodec.h>
#include <shlwapi.h>

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Secur32.lib") 
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdi32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define ID_EXIT 1

SOCKET g_socket = INVALID_SOCKET;
NOTIFYICONDATA nid = {};
HMENU g_trayMenu = NULL;

std::string GetDomainName() {
    DWORD bufferSize = 0;
    if (!GetComputerNameEx(ComputerNameDnsDomain, nullptr, &bufferSize)) {
        if (GetLastError() != ERROR_MORE_DATA) {
            return "";
        }
    }

    std::vector<wchar_t> buffer(bufferSize);
    if (!GetComputerNameEx(ComputerNameDnsDomain, buffer.data(), &bufferSize)) {
        return "";
    }

    return std::wstring(buffer.data()).begin() != std::wstring(buffer.data()).end()
        ? std::string(buffer.data(), buffer.data() + wcslen(buffer.data()))
        : "";
}

// Функция для получения имени машины
std::string GetMachineName() {
    DWORD bufferSize = MAX_COMPUTERNAME_LENGTH + 1;
    std::vector<wchar_t> buffer(bufferSize);
    if (!GetComputerNameEx(ComputerNamePhysicalNetBIOS, buffer.data(), &bufferSize)) {
        return "";
    }

    std::wstring wideStr(buffer.data());
    if (!wideStr.empty()) {
        return std::string(wideStr.begin(), wideStr.end());
    }
    else {
        return "";
    }
}

// Функция для получения IP-адреса
std::string GetIPAddress() {
    PIP_ADAPTER_ADDRESSES pAddr = nullptr;
    ULONG outBufLen = 15000;

    pAddr = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddr, &outBufLen) == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES adapter = pAddr; adapter != nullptr; adapter = adapter->Next) {
            for (PIP_ADAPTER_UNICAST_ADDRESS addr = adapter->FirstUnicastAddress; addr != nullptr; addr = addr->Next) {
                sockaddr_in* sa = (sockaddr_in*)addr->Address.lpSockaddr;
                if (sa != nullptr && sa->sin_family == AF_INET) {
                    char ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
                    free(pAddr);
                    return std::string(ip);
                }
            }
        }
    }
    free(pAddr);
    return "";
}

// Функция для получения имени пользователя
std::string GetUserName() {
    DWORD bufferSize = 0;
    EXTENDED_NAME_FORMAT nameFormat = NameSamCompatible;

    if (!GetUserNameEx(nameFormat, nullptr, &bufferSize)) {
        return "";
    }

    std::vector<wchar_t> buffer(bufferSize);
    if (!GetUserNameEx(nameFormat, buffer.data(), &bufferSize)) {
        return "";
    }

    std::wstring wideStr(buffer.data());
    if (!wideStr.empty()) {
        return std::string(wideStr.begin(), wideStr.end());
    }
    else {
        return "";
    }
}

struct ClientInfo {
    std::string domain;
    std::string machine;
    std::string ip;
    std::string user;
};

ClientInfo CollectClientInfo() {
    ClientInfo info;
    info.domain = GetDomainName();
    info.machine_name = GetMachineName();
    info.ip = GetIPAddress();
    info.user_name = GetUserName();
    return info;
}



void UpdateTrayMenuStatus() {
    if (g_trayMenu) {
        if (g_socket != INVALID_SOCKET) {
            ModifyMenu(g_trayMenu, 2, MF_BYCOMMAND, 2, L"Connected");
            nid.hIcon = LoadIcon(NULL, IDI_SHIELD);
        }
        else {
            ModifyMenu(g_trayMenu, 2, MF_BYCOMMAND, 2, L"Disconnected");
            nid.hIcon = LoadIcon(NULL, IDI_WARNING);
        }
        nid.uFlags = NIF_ICON;
        Shell_NotifyIcon(NIM_MODIFY, &nid);
        DrawMenuBar(nid.hWnd);
    }
}

bool ConnectToServer(const char* serverIp, int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//        std::cerr << "Failed to initialize Winsock." << std::endl;
        return false;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
//        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverIp, &serverAddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
//        std::cerr << "Failed to connect to server." << std::endl;
        closesocket(sock);
        WSACleanup();
        return false;
    }

    g_socket = sock;

    return true;
}

void DisconnectFromServer() {
    if (g_socket != INVALID_SOCKET) {
        shutdown(g_socket, SD_BOTH);
        closesocket(g_socket);
        g_socket = INVALID_SOCKET;
//        std::cout << "Disconnected from server." << std::endl;
    }
    WSACleanup();
}


void captureScreenshot() {
    // TODO: Implement screenshot capture using GDI32.dll
    // Encode the screenshot into a BMP or other format and send it to the server
}

void SendHeartbeat() {
    if (g_socket != INVALID_SOCKET) {
        const char* message = "HEARTBEAT";
        send(g_socket, message, strlen(message), 0);
    }
    else {
        DisconnectFromServer();
        ConnectToServer(SERVER_IP, SERVER_PORT);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_USER + 1;
        wcscpy_s(nid.szTip, sizeof(nid.szTip), TEXT("Background Client"));
        nid.hIcon = LoadIcon(NULL, IDI_INFORMATION);;

        Shell_NotifyIcon(NIM_ADD, &nid);

        g_trayMenu = CreatePopupMenu();
        AppendMenu(g_trayMenu, MF_STRING, 2, TEXT("Status: Disconnected"));
        AppendMenu(g_trayMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(g_trayMenu, MF_STRING, ID_EXIT, TEXT("Exit"));

        ConnectToServer(SERVER_IP, SERVER_PORT);

        SetTimer(hwnd, 1, 5000, NULL);
        break;
    }

    case WM_TIMER: {
        if (wParam == 1) {
            UpdateTrayMenuStatus();
            SendHeartbeat();
        }
        break;
    }

    case WM_USER+1: {
        if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);

            SetForegroundWindow(nid.hWnd);
            TrackPopupMenu(g_trayMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
        }
        break;
    }

    case WM_COMMAND: {
        if (LOWORD(wParam) == ID_EXIT) {
            if (MessageBox(NULL, TEXT("Завершить работу?"), TEXT("BackgroundClient"), MB_YESNO) == IDYES)
                DestroyWindow(hwnd);
        }
        break;
    }

    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t* className = L"BackgroundClient";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, TEXT("Failed to register window class."), TEXT("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindow(className, className, 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    if (!hwnd) {
        MessageBox(NULL, TEXT("Failed to create window."), TEXT("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, SW_HIDE);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

}