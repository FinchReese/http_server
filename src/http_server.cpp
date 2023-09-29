#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <unistd.h>
#include <http_server.h>

const unsigned int MAX_READ_BUFF_LEN = 1024; // 每次从客户端最多读取1024字节数据到缓冲区

HttpServer::HttpServer(const char *ipAddr, const unsigned short int portId,  const unsigned int backlog, const int epollSize)
    : m_ipAddr(ipAddr), m_port(portId), m_server(-1), m_backlog(backlog), m_efd(-1), m_epollSize(epollSize)
{}

HttpServer::~HttpServer()
{
    if (m_server != -1) {
        close(m_server);
    }
    if (m_efd != -1) {
        close(m_efd);
    }
}

void HttpServer::Init()
{
    if (InitServer() == false) {
        return;
    }

    if (InitEpollFd() == false) {
        return;
    }

    if (RegisterServerReadEvent() == false) {
        return;
    }

    EventLoop();
}

bool HttpServer::InitServer()
{
    if (m_server != -1) {
        printf("EVENT  Server alreadly exists.\n");
        return true;
    }

    m_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_server == -1) {
        printf("ERROR  Create socket fail.\n");
        return false;
    }

    in_addr_t ipNum = inet_addr(m_ipAddr.c_str());
    if (ipNum == INADDR_NONE) {
        printf("ERROR  Invalid ip address.\n");
        return false;
    }

    struct sockaddr_in server_addr = { 0 };
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = ipNum;
    server_addr.sin_port = htons(m_port);
    if (bind(m_server, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) == -1) {
        close(m_server);
        m_server = -1;
        printf("ERROR  server bind fail: %s:%hu.\n", m_ipAddr.c_str(), m_port);
        return false;
    }

    if (listen(m_server, m_backlog) == -1) {
        close(m_server);
        m_server = -1;
        printf("ERROR  server listen fail.\n");
        return false;
    }

    return true;
}

bool HttpServer::InitEpollFd()
{
    if (m_efd != -1) {
        return true;
    }

    m_efd = epoll_create(m_epollSize);
    if (m_efd == -1) {
        printf("ERROR  epoll_create fail.\n");
        return false;
    }

    return true;
}

bool HttpServer::RegisterServerReadEvent()
{
    struct epoll_event serverEvent = { 0 };
    serverEvent.events = EPOLLIN;
    serverEvent.data.fd = m_server;
    int ret = epoll_ctl(m_efd, EPOLL_CTL_ADD, m_server, &serverEvent);
    if (ret == -1) {
        printf("ERROR  Register server read event fail.\n");
        return false;
    }

    return true;
}

void HttpServer::EventLoop()
{
    struct epoll_event *events = new struct epoll_event[m_epollSize];
    if (events == nullptr) {
        printf("ERROR  Allooc epoll_event memory fail.\n");
        return;
    }

    bool stopFlag = false;
    while (!stopFlag) {
        int ret = epoll_wait(m_efd, events, m_epollSize, -1);
        if (ret == -1) {
            delete[] events;
            return;
        }
        for (unsigned int i = 0; i < static_cast<unsigned int>(ret); ++i) {
            if (events[i].events & EPOLLIN == 0) {
                continue;
            }
            int socket = events[i].data.fd;
            if (socket == m_server) {
                HandleServerReadEvent();
            } else {
                HandleClientReadEvent(socket);
            }
        }
    }

    delete []events;
    return;
}

void HttpServer::HandleServerReadEvent()
{
    struct sockaddr_in clientAddr = { 0 };
    socklen_t clientAddrLen = sizeof(clientAddr);
    int client = accept(m_server, reinterpret_cast<struct sockaddr *>(&clientAddr), &clientAddrLen);
    if (client == -1) {
        printf("ERROR  accept fail.\n");
        return;
    }
    printf("EVENT  new connect: client[%d] with %s:%hu.\n", client, inet_ntoa(clientAddr.sin_addr),
        ntohs(clientAddr.sin_port));

    // 注册客户端的监听读事件
    struct epoll_event clientEvent = { 0 };
    clientEvent.events = EPOLLIN;
    clientEvent.data.fd = client;
    int ret = epoll_ctl(m_efd, EPOLL_CTL_ADD, client, &clientEvent);
    if (ret == -1) {
        printf("ERROR  epoll_ctl fail.\n");
        close(client);
        return;
    }
}

void HttpServer::HandleClientReadEvent(const int client)
{
    char readBuff[MAX_READ_BUFF_LEN + 1] = { 0 };
    ssize_t n = read(client, readBuff, MAX_READ_BUFF_LEN);
    if (n <= 0) {
        int ret = epoll_ctl(m_efd, EPOLL_CTL_DEL, client, NULL);
        if (ret == -1) {
            return;
        }
        close(client);
    } else {
        // 调试时，只打印收到的信息，先不回复
        struct sockaddr_in clientAddr = { 0 };
        socklen_t clientAddrLen = sizeof(clientAddr);
        int ret = getsockname(client, reinterpret_cast<struct sockaddr *>(&clientAddr), &clientAddrLen);
        if (ret == -1) {
            printf("DEBUG  client recv msg:\n%s\n", readBuff);
        } else {
        printf("DEBUG  client %s:%hu recv msg:\n%s\n", inet_ntoa(clientAddr.sin_addr),
            ntohs(clientAddr.sin_port), readBuff);
        }

    }
}