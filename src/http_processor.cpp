#include <string.h>
#include "http_processor.h"

const char *WHITE_SPACE_CHARS = " \t";

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
    if (GetFieldSplitedByWhiteSpaceChars(m_url) == false) {
        printf("ERROR Get method fail.\n");
        return false;
    }
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