#include <string.h>
#include "http_processor.h"

const char *WHITE_SPACE_CHARS = " \t";

HttpProcessor::HttpProcessor(socketId) : m_socketId(socketId), m_currentRequestSize(0),
    m_startIndex(m_request), m_currentIndex(0), m_processState(HTTP_PROCESS_STATE_PARSE_REQUEST_LINE)
{
    (void)memset_s(m_request, sizeof(m_request), 0, sizeof(m_request));
    (void)memset_s(m_method, sizeof(m_method), 0, sizeof(m_method));
}

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

bool HttpProcessor::ParseMethod()
{
    char *ret = strpbrk(m_startIndex, WHITE_SPACE_CHARS);
    if (ret == nullptr) {
        printf("ERROR Can't find white-space char: %s\n", m_startIndex);
        return false;
    }
    *ret = '\0';
    errno_t err = strcpy_s(m_method, sizeof(m_method), m_startIndex);
    if (err != EOK) {
        printf("ERROR copy method fail, origin string is %s", m_startIndex);
        return false;
    }
    ret++;
    m_startIndex = ret + strspn(ret, whiteSpaceChars);
}