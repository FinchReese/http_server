#ifndef HTTP_SERVER_H
#define HTTP_SERVER

#include <string.h>

class HttpServer {
public:
    HttpServer(const std::string ipAddr, const unsigned short int portId, const unsigned int backlog);
    ~HttpServer();
    void Init();
private:
    bool InitServer();
    bool InitEpollFd();
    bool RegisterServerReadEvent();
    void EventLoop();
    void HandleServerReadEvent();
    void HandleClientReadEvent(const int client);
private:
    // 服务器socket相关参数
    std::string m_ipAddr;
    unsigned short int m_port;
    int m_server;
    unsigned int m_backlog;
    // epoll相关参数
    int m_efd;
    int m_epollSize;
};

#endif