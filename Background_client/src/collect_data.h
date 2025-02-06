#pragma once
struct ClientInfo {
    std::string domain;
    std::string machine;
    std::string ip;
    std::string user;
};
ClientInfo CollectClientInfo();