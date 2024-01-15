#ifndef HTTP_PROCESSOR_H
#define HTTP_PROCESSOR_H

const unsigned int MAX_RECV_BUFF_LEN = 2048;

enum GetALineStatus : unsigned char {
    GET_A_LINE_OK = 0,
    GET_A_LINE_CONTINUE = 1,
    GET_A_LINE_WRROR = 2,
};

class HttpProcessor {
public:
    HttpProcessor(const int socketId);
    ~HttpProcessor();
    bool Read();
    bool ProcessRequest();

private:
    GetALineStatus GetALine();
    char m_request[MAX_RECV_BUFF_LEN]; // 记录请求报文
    int m_socketId; // 对应的套接字id
    unsigned int m_currentRequestSize; // 记录当前收到的请求报文长度
    unsigned int m_startIndex; // 解析报文的起始位置，是报文中每行的首字节
    unsigned int m_currentIndex; // 解析报文的当前位置
};


#endif