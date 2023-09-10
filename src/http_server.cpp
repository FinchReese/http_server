#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <http_server.h>

HttpServer::HttpServer(const std::string ipAddr, const unsigned short int portId,  const unsigned int backlog)
    : m_ipAddr(ipAddr), m_port(portId), m_server(-1), m_backlog(backlog)
{}

HttpServer::~HttpServer()
{}

bool HttpServer::Init()
{
    if (InitServer == false) {
        return false;
    }

    return true;
}

bool HttpServer::InitServer()
{
    if (m_server != -1) {
        printf("EVENT  Server alreadly exists.");
        return true;
    }

    m_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_server == -1) {
        printf("ERROR  Create socket fail.");
        return false;
    }

    in_addr_t ipNum = inet_addr(m_ipAddr.c_str());
    if (ipNum == INADDR_NONE) {
        printf("ERROR  Invalid ip address.");
        return false;
    }

    struct sockaddr_in server_addr = { 0 };
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = ipNum;
    server_addr.sin_port = htons(m_port);
    if (bind(m_server, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) == -1) {
        close(m_server);
        m_server = -1;
        printf("ERROR  server bind fail: %s:%hu", m_ipAddr.c_str(), m_port);
        return false;
    }

    if (listen(m_server, m_backlog) == -1) {
        close(m_server);
        m_server = -1;
        printf("ERROR  server listen fail.");
        return false;
    }

    return true;
}