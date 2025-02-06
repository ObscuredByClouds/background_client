#include "include.h"
#include <sstream>

#include <fstream>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define ID_EXIT 1

SOCKET g_socket = INVALID_SOCKET;
NOTIFYICONDATA nid = {};
HMENU g_trayMenu = NULL;

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


    const wchar_t* className = TEXT("BackgroundClient");

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


    std::ofstream ofs("example.txt");
    ClientInfo clientInfo = CollectClientInfo();

    ofs << "Domain: " << (clientInfo.domain.empty() ? "N/A" : clientInfo.domain) << std::endl;
    ofs << "Machine Name: " << (clientInfo.machine.empty() ? "N/A" : clientInfo.machine) << std::endl;
    ofs << "IP Address: " << (clientInfo.ip.empty() ? "N/A" : clientInfo.ip) << std::endl;
    ofs << "User Name: " << (clientInfo.user.empty() ? "N/A" : clientInfo.user) << std::endl;

    ofs.close();
    return 0;

    return 0;
    ShowWindow(hwnd, SW_HIDE);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

}