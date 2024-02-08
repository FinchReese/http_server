#include <string.h>
#include "http_processor.h"

const char *WHITE_SPACE_CHARS = " \t";
const char *GET_METHOD_STR = "GET";
const char *URL_HTTP_PREFIX = "http://";
const char URL_SPLIT_CHAR = '/';
const char END_CHAR = '\0';
const char *CONTENT_LENGTH_KEY_NAME = "Content-Length";
const char *CONNECTION_KEY_NAME = "Connection";
const char *KEEP_ALIVE_VALUE = "keep-alive";

void (HttpProcessor::*ParseHeadFieldValueStr)();
typedef struct {
    const char *keyName;
    ParseHeadFieldValueStr parseFunc;
} KeyNameAndParseFuncMap;

const KeyNameAndParseFuncMap KEY_NAME_AND_PARSE_FUNC_MAP_LIST[] = {
    { CONTENT_LENGTH_KEY_NAME, HttpProcessor::ParseContentLength },
    { CONNECTION_KEY_NAME, HttpProcessor::ParseConnection },
};

const KeyNameAndParseFuncMap KEY_NAME_AND_PARSE_FUNC_MAP_LIST_SIZE =
    sizeof(KEY_NAME_AND_PARSE_FUNC_MAP_LIST) / sizeof(KEY_NAME_AND_PARSE_FUNC_MAP_LIST[0]);

HttpProcessor::HttpProcessor(socketId) : m_socketId(socketId)
{}

HttpProcessor::~HttpProcessor()
{}

bool HttpProcessor::Read()
{
    ssize_t readSize = read(m_socketId, m_request + m_currentRequestSize, MAX_RECV_BUFF_LEN - m_currentRequestSize);
    if (readSize <= 0) {
        printf("ERROR read fail, socket id = %d\n", m_socketId);
        return false;
    }

    m_currentRequestSize += readSize;
    return true;
}

bool HttpProcessor::ProcessRequest()
{

}

GetALineStatus HttpProcessor::GetALine()
{
    if (m_currentIndex >= m_currentRequestSize) {
        return GET_A_LINE_CONTINUE;
    }

    while (m_currentIndex < m_currentRequestSize) {
        switch (m_request[m_currentIndex]) {
            case '\r': {
                if (m_currentIndex + 1 < m_currentRequestSize) {
                    if (m_request[m_currentIndex + 1] == '\n') {
                        m_request[m_currentIndex++] = '\0';
                        m_request[m_currentIndex++] = '\0';
                        return GET_A_LINE_OK;
                    } else {
                        return GET_A_LINE_ERROR;
                    }
                } else {
                    return GET_A_LINE_CONTINUE;
                }
            }
            case '\n': {
                return GET_A_LINE_ERROR;
            }
            default: {
                m_currentIndex++;
                break;
            }
        }
    }
    return GET_A_LINE_CONTINUE;
}

ParseRequestReturnCode HttpProcessor::ParseRequestLine()
{
    if (GetFieldSplitedByWhiteSpaceChars(m_method) == false) {
        printf("ERROR Get method fail.\n");
        return PARSE_REQUEST_RETURN_CODE_ERROR;
    }
    if (strcasecmp(m_httpVersion, GET_METHOD_STR) != 0) {
        printf("ERROR Support GET method only.\n");
        return PARSE_REQUEST_RETURN_CODE_ERROR;
    }

    if (GetFieldSplitedByWhiteSpaceChars(m_url) == false) {
        printf("ERROR Get url fail.\n");
        return PARSE_REQUEST_RETURN_CODE_ERROR;
    }
    printf("EVENT Origin url: %s.\n", m_url);
    if (strncasecmp(m_url, URL_HTTP_PREFIX, strlen(URL_HTTP_PREFIX)) == 0) {
        m_url += strlen(URL_HTTP_PREFIX);
        m_url = strchr(m_url, URL_SPLIT_CHAR);
    }
    if (m_url == nullptr || m_url[0] != URL_SPLIT_CHAR) {
        printf("ERROR Invalid url.\n");
        return PARSE_REQUEST_RETURN_CODE_ERROR;
    }
    
    m_httpVersion = m_startIndex;
    if (strcasecmp(m_httpVersion, "HTTP/1.0") != 0) {
        printf("ERROR Support HTTP/1.0 only.\n");
        return PARSE_REQUEST_RETURN_CODE_ERROR;
    }
    return PARSE_REQUEST_RETURN_CODE_CONTINUE;
}

bool HttpProcessor::GetFieldSplitedByWhiteSpaceChars(char *&field)
{
    char *ret = strpbrk(m_startIndex, WHITE_SPACE_CHARS);
    if (ret == nullptr) {
        printf("ERROR Can't find white-space char: %s\n", m_startIndex);
        return false;
    }
    *ret = '\0';
    field = m_startIndex;
    ret++;
    m_startIndex = ret + strspn(ret, whiteSpaceChars);
    return true;
}

ParseRequestReturnCode HttpProcessor::ParseHeadFields()
{
    if (m_startIndex == END_CHAR) {
        if (m_contentLen != 0) {
            m_processState = HTTP_PROCESS_STATE_PARSE_MESSAGE_BODY;
            return ARSE_REQUEST_RETURN_CODE_CONTINUE;
        }
        return PARSE_REQUEST_RETURN_CODE_FINISH;
    }

    for (unsigned int i = 0; i < KEY_NAME_AND_PARSE_FUNC_MAP_LIST_SIZE; ++i) {
        KeyNameAndParseFuncMap map = KEY_NAME_AND_PARSE_FUNC_MAP_LIST[i];
        if (strncasecmp(m_startIndex, map.keyName, strlen(map.keyName)) == 0) {
            m_startIndex += strlen(map.keyName);
            m_startIndex += 1; // 跳过':'
            m_startIndex += strspn(m_startIndex, "\t ");
            map.parseFunc();
        }
    }
    return PARSE_REQUEST_RETURN_CODE_CONTINUE;
}

void HttpProcessor::ParseContentLength()
{
    m_contentLen = atol(m_startIndex);
}

void HttpProcessor::ParseConnection()
{
    if (strcasecmp(m_startIndex, KEEP_ALIVE_VALUE) == 0) {
        m_keepAlive = true;
    }
}