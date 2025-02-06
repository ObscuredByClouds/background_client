#include "include.h"

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

    std::wstring wideStr(buffer.data());
    if (!wideStr.empty()) {
        return std::string(wideStr.begin(), wideStr.end());
    }
    else {
        return "";
    }
}

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

ClientInfo CollectClientInfo() {
    ClientInfo info;
    info.domain = GetDomainName();
    info.machine = GetMachineName();
    info.ip = GetIPAddress();
    info.user = GetUserName();
    return info;
}