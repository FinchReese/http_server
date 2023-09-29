#ifndef HTTP_SERVER_H
#define HTTP_SERVER

#include <string>

class HttpServer {
public:
    static HttpServer *GetInstance();
    void Init(const char *ipAddr, const unsigned short int portId,  const unsigned int backlog,
        const int epollSize);
private:
    HttpServer();
    ~HttpServer();
    bool InitServer(const char *ipAddr, const unsigned short int portId,  const unsigned int backlog);
    bool InitEpollFd(const int epollSize);
    bool RegisterServerReadEvent();
    void EventLoop(const int epollSize);
    void HandleServerReadEvent();
    void HandleClientReadEvent(const int client);
private:
    int m_server;
    int m_efd;
};

#endif