#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <commctrl.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shell32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define ID_EXIT 1

SOCKET g_socket = INVALID_SOCKET;
HICON g_hIcon = NULL;
NOTIFYICONDATA nid = {};
//bool g_isConnected = false;
HMENU g_trayMenu = NULL;

void UpdateTrayMenuStatus() {
    if (g_trayMenu) {
        if(g_socket != INVALID_SOCKET) {
            ModifyMenu(g_trayMenu, 2, MF_BYCOMMAND, 2, "Connected");
        }
        else {
            ModifyMenu(g_trayMenu, 2, MF_BYCOMMAND, 2, "Disconnected");

        }
        DrawMenuBar(nid.hWnd);
    }
}

bool ConnectToServer(const char* serverIp, int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return false;
    }

    SOCKET sock  = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket." << std::endl;
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverIp, &serverAddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Failed to connect to server." << std::endl;
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
        std::cout << "Disconnected from server." << std::endl;
    }
    WSACleanup();
}

std::string getDomainName() {
    char domainName[256];
    DWORD size = sizeof(domainName);
    if (GetComputerNameExA(ComputerNameDnsDomain, domainName, &size)) {
        return std::string(domainName);
    }
    return "Unknown";
}

std::string getMachineName() {
    char machineName[256];
    DWORD size = sizeof(machineName);
    if (GetComputerNameA(machineName, &size)) {
        return std::string(machineName);
    }
    return "Unknown";
}

std::string getUserName() {
    char userName[256];
    DWORD size = sizeof(userName);
    if (GetUserNameA(userName, &size)) {
        return std::string(userName);
    }
    return "Unknown";
}

std::string getIPAddress() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        return "Unknown";
    }

    struct hostent* host = gethostbyname(hostname);
    if (host == nullptr) {
        return "Unknown";
    }

    struct in_addr addr;
    memcpy(&addr, host->h_addr_list[0], sizeof(struct in_addr));
    return std::string(inet_ntoa(addr));
}

void sendMessage(SOCKET sock, const char* message) {
    if (send(sock, message, strlen(message), 0) == SOCKET_ERROR)
        std::cerr << "Failed to send data." << std::endl;
}

void captureScreenshot() {
    // TODO: Implement screenshot capture using GDI32.dll
    // Encode the screenshot into a BMP or other format and send it to the server
}

void SendHeartbeat() {
    if (g_socket != INVALID_SOCKET) {
        const char* message = "HEARTBEAT";
        send(g_socket, message, strlen(message), 0);
    } else {
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
            strcpy_s(nid.szTip, sizeof(nid.szTip), "Background Client");
            g_hIcon = LoadIcon(NULL, IDI_INFORMATION);
            nid.hIcon = g_hIcon;

            Shell_NotifyIcon(NIM_ADD, &nid);

            g_trayMenu = CreatePopupMenu();
            AppendMenu(g_trayMenu, MF_STRING, 2, "Status: Disconnected");
            AppendMenu(g_trayMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(g_trayMenu, MF_STRING, ID_EXIT, "Exit");

            ConnectToServer(SERVER_IP, SERVER_PORT);

            SetTimer(hwnd, 1, 5000, NULL);
            break;
        }

        case WM_TIMER: {
            if (wParam == 1) {
                SendHeartbeat();
            }
            break;
        }

        case WM_USER + 1: {
            if (lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);

                UpdateTrayMenuStatus();
                SetForegroundWindow(nid.hWnd);
                TrackPopupMenu(g_trayMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            }
            break;
        }
        
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_EXIT) {
                Shell_NotifyIcon(NIM_DELETE, &nid);

                if (g_hIcon) {
                    DestroyIcon(g_hIcon);
                    g_hIcon = NULL;
                }

                DisconnectFromServer();
                KillTimer(hwnd, 1);
                PostQuitMessage(0);
                break;
            }
            break;

        case WM_QUERYENDSESSION:
            return TRUE;

        case WM_ENDSESSION: {
            if (wParam) {
                Shell_NotifyIcon(NIM_DELETE, &nid);

                if (g_hIcon) {
                    DestroyIcon(g_hIcon);
                    g_hIcon = NULL;
                }

                DisconnectFromServer();
                KillTimer(hwnd, 1);
                PostQuitMessage(0);
                break;
            }
        }

        case WM_DESTROY: {
            Shell_NotifyIcon(NIM_DELETE, &nid);

            if (g_hIcon) {
                DestroyIcon(g_hIcon);
                g_hIcon = NULL;
            }

            DisconnectFromServer();
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            break;
        }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char* className = "BackgroundClient";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "Failed to register window class.", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindow(className, className, 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    if (!hwnd) {
        MessageBox(NULL, "Failed to create window.", "Error", MB_OK | MB_ICONERROR);
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