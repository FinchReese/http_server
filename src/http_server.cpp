#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "http_server.h"

const unsigned int MAX_READ_BUFF_LEN = 1024; // 每次从客户端最多读取1024字节数据到缓冲区
const time_t TIMER_INTERVAL = 5; // 定时器时间间隔定为5S
const time_t CLIENT_EXPIRE_THRESHOLD = 15; // 客户端超时时间定为15S
const unsigned int LIMIT_FD = 65535; // 文件描述符最多有65535个

int HttpServer::m_pipefd[2] = { -1, -1 };

HttpServer::HttpServer() : m_server(-1), m_efd(-1), m_clientExpireHeap(LIMIT_FD)
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

HttpServer *HttpServer::GetInstance()
{
    static HttpServer httpServer;
    return &httpServer;
}

void HttpServer::Init(const char *ipAddr, const unsigned short int portId,  const unsigned int backlog,
    const int epollSize)
{
    if (InitServer(ipAddr, portId, backlog) == false) {
        return;
    }

    if (InitEpollFd(epollSize) == false) {
        return;
    }

    if (RegisterServerReadEvent() == false) {
        return;
    }

    if (InitPipeFd() == false) {
        return;
    }
    // 注册SIGTERM信号的监听
    if (RegisterListenSignal(SIGALRM) == false) {
        ClosePipeFd();
        return;
    }
    // 注册SIGTERM信号的监听
    if (RegisterListenSignal(SIGTERM) == false) {
        ClosePipeFd();
        return;
    }
    // 注册m_pipefd[1]读事件的监听
    if (RegisterPipeReadEvent() == false) {
        ClosePipeFd();
        return;
    }
    EventLoop(epollSize);
    ClosePipeFd();
}

bool HttpServer::InitServer(const char *ipAddr, const unsigned short int portId,  const unsigned int backlog)
{
    if (m_server != -1) {
        printf("ERROR  Server alreadly exists.\n");
        return false;
    }

    m_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_server == -1) {
        printf("ERROR  Create socket fail.\n");
        return false;
    }

    if (ipAddr == nullptr) {
        printf("ERROR  ipAddr is null.\n");
        return false;
    }
    in_addr_t ipNum = inet_addr(ipAddr);
    if (ipNum == INADDR_NONE) {
        close(m_server);
        m_server = -1;       
        printf("ERROR  Invalid ip address: %s.\n", ipAddr);
        return false;
    }

    struct sockaddr_in server_addr = { 0 };
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = ipNum;
    server_addr.sin_port = htons(portId);
    if (bind(m_server, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr)) == -1) {
        close(m_server);
        m_server = -1;
        printf("ERROR  server bind fail: %s:%hu.\n", ipAddr, portId);
        return false;
    }

    if (listen(m_server, backlog) == -1) {
        close(m_server);
        m_server = -1;
        printf("ERROR  server listen fail.\n");
        return false;
    }

    return true;
}

bool HttpServer::InitEpollFd(const int epollSize)
{
    if (m_efd != -1) {
        printf("ERROR  Epoll alreadly exists.\n");
        return false;
    }

    m_efd = epoll_create(epollSize);
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

bool HttpServer::InitPipeFd()
{
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_pipefd);
    if (ret == -1) {
        return false;
    }
    return true;
}

void HttpServer::ClosePipeFd()
{
    close(m_pipefd[0]);
    m_pipefd[0] == -1;
    close(m_pipefd[1]);
    m_pipefd[1] == -1;
}

bool HttpServer::RegisterListenSignal(const int signalId)
{
    struct sigaction sa = { 0 };
    sa.sa_handler = WriteSignalToPipeFd;
    sigfillset(&sa.sa_mask);
    sa.sa_flags |= SA_RESTART;
    if (sigaction(signalId, &sa, NULL) == -1) {
        return false;
    }
    return true;
}

void HttpServer::WriteSignalToPipeFd(int signalId)
{
    int tmpErrno = errno;
    ssize_t ret = write(m_pipefd[0], reinterpret_cast<const int *>(&signalId), sizeof(signalId));
    printf("DEBUG  WriteSignalToPipeFd signalId=%d, ret=%ld, tmpErrno=%d, errno=%d\n",
        signalId, ret, tmpErrno, errno);
    errno = tmpErrno;
}

bool HttpServer::RegisterPipeReadEvent()
{
    struct epoll_event pipeEvent = { 0 };
    pipeEvent.events = EPOLLIN;
    pipeEvent.data.fd = m_pipefd[1];
    int ret = epoll_ctl(m_efd, EPOLL_CTL_ADD, m_pipefd[1], &pipeEvent);
    if (ret == -1) {
        printf("ERROR  Register pipe read event fail.\n");
        return false;
    }
    return true;
}

void HttpServer::EventLoop(const int epollSize)
{
    // 启动alarm定时器
    alarm(TIMER_INTERVAL);
    struct epoll_event *events = new struct epoll_event[epollSize];
    if (events == nullptr) {
        printf("ERROR  Allooc epoll_event memory fail.\n");
        return;
    }

    bool stopFlag = false;
    bool timeout = false;
    while (!stopFlag) {
        int ret = epoll_wait(m_efd, events, epollSize, -1);
        if (ret == -1) {
            printf("ERROR  epoll_wait fail, errno = %d.\n", errno);
            if (errno == EINTR) {
                continue;
            } else {
                delete[] events;
                return;
            }
        }
        for (unsigned int i = 0; i < static_cast<unsigned int>(ret); ++i) {
            if (events[i].events & EPOLLIN == 0) {
                continue;
            }
            int socket = events[i].data.fd;
            if (socket == m_server) {
                HandleServerReadEvent();
            } else if (socket == m_pipefd[1]) {
                HandleSignalEvent(stopFlag, timeout);
            } else {
                HandleClientReadEvent(socket);
            }
        }
        if (timeout) {
            HandleTimeoutEvent();
            alarm(TIMER_INTERVAL);
            timeout = false;
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
    // 将客户端超时信息写入最小堆
    time_t expire;
    expire = time(NULL);
    ClientExpire clientExpire(client, expire + CLIENT_EXPIRE_THRESHOLD);
    bool ret = m_clientExpireHeap.push(clientExpire);
    if (!ret) {
        epoll_ctl(m_efd, EPOLL_CTL_DEL, client, NULL);
        printf("ERROR  push clientExpire fail.\n");
        close(client);      
    }
}

void HttpServer::HandleSignalEvent(bool &stopFlag, bool &timeout)
{
    const unsigned int maxSignalNum = 1024; // 读取信号数量最大值为1024
    int signalList[maxSignalNum] = { 0 };
    ssize_t readBytes = read(m_pipefd[1], signalList, maxSignalNum);
    if (readBytes == -1) {
        return;
    }
    for (unsigned int i = 0; i < static_cast<unsigned int>(readBytes) / sizeof(int); i++) {
        int signalId = signalList[i];
        switch (signalId) {
            case SIGTERM: {
                stopFlag = true;
                break;
            }
            case SIGALRM: {
                timeout = true;
                break;
            }
            default: {
                break;
            }
        }
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
        // 需要更新最小堆中对应节点的expire
    }
}

void HttpServer::HandleTimeoutEvent()
{
    time_t currentExpire;
    currentExpire = time(NULL);
    ClientExpire tmp;
    while (m_clientExpireHeap.top(tmp)) {
        if (tmp.m_expire >= currentExpire) {
            break;
        }
        (void)m_clientExpireHeap.pop(tmp);
    }
}