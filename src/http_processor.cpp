#include <string.h>
#include "http_processor.h"

HttpProcessor::HttpProcessor(socketId) : m_socketId(socketId), m_currentRequestSize(0),
    m_startIndex(0), m_currentIndex(0)
{
    (void)memset(m_request, 0, sizeof(m_request));
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