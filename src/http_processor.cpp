#include <string.h>
#include "http_processor.h"

const char *WHITE_SPACE_CHARS = " \t";
const char *GET_METHOD_STR = "GET";
const char *URL_HTTP_PREFIX = "http://";
const char URL_SPLIT_CHAR = '/';

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

bool HttpProcessor::ParseRequestLine()
{
    if (GetFieldSplitedByWhiteSpaceChars(m_method) == false) {
        printf("ERROR Get method fail.\n");
        return false;
    }
    if (strcasecmp(m_httpVersion, GET_METHOD_STR) != 0) {
        printf("ERROR Support GET method only.\n");
        return false;
    }

    if (GetFieldSplitedByWhiteSpaceChars(m_url) == false) {
        printf("ERROR Get url fail.\n");
        return false;
    }
    printf("EVENT Origin url: %s.\n", m_url);
    if (strncasecmp(m_url, URL_HTTP_PREFIX, strlen(URL_HTTP_PREFIX)) == 0) {
        m_url += strlen(URL_HTTP_PREFIX);
        m_url = strchr(m_url, URL_SPLIT_CHAR);
    }
    if (m_url == nullptr || m_url[0] != URL_SPLIT_CHAR) {
        printf("ERROR Invalid url.\n");
        return false;
    }
    
    m_httpVersion = m_startIndex;
    if (strcasecmp(m_httpVersion, "HTTP/1.0") != 0) {
        printf("ERROR Support HTTP/1.0 only.\n");
        return false;
    }
    return true;
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