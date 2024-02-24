#ifndef HTTP_PROCESSOR_H
#define HTTP_PROCESSOR_H

#include <sys/uio.h>

const unsigned int MAX_READ_BUFF_LEN = 2048;
const unsigned int MAX_WRITE_BUFF_LEN = 1024;

enum GetALineState : unsigned char {
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

enum ResponseStatusCode : unsigned int {
    RESPONSE_STATUS_CODE_OK = 200, // 请求成功
    RESPONSE_STATUS_CODE_BAD_REQUEST = 400, // 通用客户请求错误
    RESPONSE_STATUS_CODE_FORBIDDEN = 403, // 访问被服务器禁止
    RESPONSE_STATUS_CODE_NOT_FOUND = 404, // 资源没找到
    RESPONSE_STATUS_CODE_INTERNAL_SERVER_ERROR = 500, // 通用服务器错误
};

enum SendResponseReturnCode {
    SEND_RESPONSE_RETURN_CODE_FINISH = 0; // 发送回复消息完成
    SEND_RESPONSE_RETURN_CODE_ERROR = 1; // 发送回复消息出错
    SEND_RESPONSE_RETURN_CODE_AGAIN = 2; // 再试一次
    SEND_RESPONSE_RETURN_CODE_NEXT = 3; // 进入下一次处理消息流程
};

enum VectorIndex {
    STATUS_LINE_AND_HEAD_FIELD_VECTOR_INDEX = 0; // 状态行和头部信息对应向量下标
    CONTENT_VECTOR_INDEX = 1; // 消息体对应向量下标
    VECTOR_COUNT,
};

class HttpProcessor {
public:
    HttpProcessor(const int socketId, const char *sourceDir);
    ~HttpProcessor();
    bool Read();
    SendResponseReturnCode Write();
    void Init();
    bool ProcessRequest();
    GetALineState GetALine();
    ParseRequestReturnCode ParseRequestLine();
    bool GetFieldSplitedByWhiteSpaceChars(char *&field);
private:
    GetALineState GetALine();
private:
    char *m_sourceDir{ nullptr };
    char m_request[MAX_READ_BUFF_LEN]{ 0 }; // 记录请求报文
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
    char m_writeBuff[MAX_WRITE_BUFF_LEN]{ 0 }; // 记录请求报文
    unsigned int m_writeSize{ 0 };
    char *m_fileAddr{ nullptr };
    unsigned int m_fileSize{ 0 };
    struct iovec m_iov[VECTOR_COUNT]{ 0 };
    int m_cnt{ 0 };
    unsigned int m_leftRespSize{ 0 }; // 剩余回复字节数
};


#endif