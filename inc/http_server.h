#ifndef HTTP_SERVER_H
#define HTTP_SERVER

#include <string.h>

class HttpServer {
public:
    HttpServer(const std::string ipAddr, const unsigned short int portId, const unsigned int backlog);
    ~HttpServer();
    bool Init();
private:
    bool InitServer();
private:
    std::string m_ipAddr;
    unsigned short int m_port;
    int m_server;
    unsigned int m_backlog;
};

#endif