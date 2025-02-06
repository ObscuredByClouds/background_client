#pragma once

#define SECURITY_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <commctrl.h>

#include <security.h>

#include <sddl.h>
#include <Sspi.h>
#include <iphlpapi.h>
#include <comdef.h>
#include <wincodec.h>
#include <shlwapi.h>

#include "collect_data.h"

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Secur32.lib") 
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdi32.lib")
