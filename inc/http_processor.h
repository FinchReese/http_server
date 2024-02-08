#ifndef HTTP_PROCESSOR_H
#define HTTP_PROCESSOR_H

const unsigned int MAX_RECV_BUFF_LEN = 2048;

enum GetALineStatus : unsigned char {
    GET_A_LINE_OK = 0,
    GET_A_LINE_CONTINUE = 1,
    GET_A_LINE_ERROR = 2,
};

enum HttpProcessState : unsigned int {
    HTTP_PROCESS_STATE_PARSE_REQUEST_LINE = 0,
    HTTP_PROCESS_STATE_PARSE_HEAD_FIELD = 1,
    HTTP_PROCESS_STATE_PARSE_MESSAGE_BODY = 2,
};

enum ParseRequestReturnCode: unsigned int {
    PARSE_REQUEST_RETURN_CODE_FINISH = 0, // 解析请求消息完成
    PARSE_REQUEST_RETURN_CODE_ERROR = 1, // 解析请求消息出错
    PARSE_REQUEST_RETURN_CODE_CONTINUE = 2, // 需要继续解析
};

class HttpProcessor {
public:
    HttpProcessor(const int socketId);
    ~HttpProcessor();
    bool Read();
    bool ProcessRequest();
    GetALineStatus GetALine();
    ParseRequestReturnCode ParseRequestLine();
    bool GetFieldSplitedByWhiteSpaceChars(char *&field);

private:
    GetALineStatus GetALine();
    char m_request[MAX_RECV_BUFF_LEN]{ 0 }; // 记录请求报文
    int m_socketId{ 0 }; // 对应的套接字id
    unsigned int m_currentRequestSize{ 0 }; // 记录当前收到的请求报文长度
    char *m_startPos{ nullptr}; // 指向解析报文字段的起始位置
    unsigned int m_currentIndex{ 0 }; // 解析报文的当前位置
    HttpProcessState m_processState{ HTTP_PROCESS_STATE_PARSE_REQUEST_LINE };
    char *m_method{ nullptr };
    char *m_url{ nullptr };
    char *m_httpVersion{ nullptr };
    unsigned int m_contentLen{ 0 };
    bool m_keepAlive{ false };
};


#endif