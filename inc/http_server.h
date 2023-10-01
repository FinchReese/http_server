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
    static bool InitPipeFd();
    static void ClosePipeFd();
    bool RegisterListenSignal(const int signalId);
    static void WriteSignalToPipeFd(int signalId);
    bool RegisterPipeReadEvent();
    void EventLoop(const int epollSize);
    void HandleServerReadEvent();
    void HandleSignalEvent(bool &stopFlag, bool &timeout);
    void HandleClientReadEvent(const int client);
private:
    int m_server;
    int m_efd;
    static int m_pipefd[2];
};

#endif