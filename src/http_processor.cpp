#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "http_processor.h"

const char *WHITE_SPACE_CHARS = " \t";
const char *GET_METHOD_STR = "GET";
const char *URL_HTTP_PREFIX = "http://";
const char URL_SPLIT_CHAR = '/';
const char HEAD_FIELD_SPLIT_CHAR = ':';
const char END_CHAR = '\0';
const char *CONTENT_LENGTH_KEY_NAME = "Content-Length";
const char *CONNECTION_KEY_NAME = "Connection";
const char *KEEP_ALIVE_VALUE = "keep-alive";
const char *CLOSE_ALIVE_VALUE = "close";
const char *HTTP_VERSION = "HTTP/1.1";
const char *OK_TITLE = "OK";
const char *BAD_REQUEST_TITLE = "Bad Request";
const char *BAD_REQUEST_CONTENT = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char *FORBIDDEN_TITLE = "Forbidden";
const char *FORBIDDEN_CONTENT = "You don't have permission to get file from this server.\n";
const char *NOT_FOUND_TITLE = "Not Found";
const char *NOT_FOUND_CONTENT = "The request file was not found on this server.\n";
const char *INTERNAL_SERVER_ERROR_TITLE = "Internal Server Error";
const char *INTERNAL_SERVER_ERROR_CONTENT = "There was an unusual problem serving the requested file.\n";
const unsigned int MAX_FILE_NAME_LEN = 200;

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

typedef struct {
    ResponseStatusCode statusCode;
    const char *statusTitle;
    const char *statusContent;
} StatusInfo;

const StatusInfo ERROR_STATUS_INFO_LIST[] = {
    { RESPONSE_STATUS_CODE_BAD_REQUEST, BAD_REQUEST_TITLE, BAD_REQUEST_CONTENT },
    { RESPONSE_STATUS_CODE_FORBIDDEN, FORBIDDEN_TITLE, FORBIDDEN_CONTENT },
    { RESPONSE_STATUS_CODE_NOT_FOUND, NOT_FOUND_TITLE, NOT_FOUND_CONTENT },
    { RESPONSE_STATUS_CODE_INTERNAL_SERVER_ERROR, INTERNAL_SERVER_ERROR_TITLE, INTERNAL_SERVER_ERROR_CONTENT },
};
const unsigned int ERROR_STATUS_INFO_LIST_SIZE = sizeof(ERROR_STATUS_INFO_LIST) / sizeof(ERROR_STATUS_INFO_LIST[0]);

HttpProcessor::HttpProcessor(socketId, const char *sourceDir) : m_socketId(socketId), m_sourceDir(sourceDir)
{}

HttpProcessor::~HttpProcessor()
{}

bool HttpProcessor::Read()
{
    ssize_t readSize = read(m_socketId, m_request + m_currentRequestSize, MAX_READ_BUFF_LEN - m_currentRequestSize);
    if (readSize <= 0) {
        printf("ERROR read fail, socket id = %d\n", m_socketId);
        return false;
    }

    m_currentRequestSize += readSize;
    return true;
}

ParseRequestReturnCode HttpProcessor::ParseRequest()
{
    GetALineState getALineState;
    ParseRequestReturnCode ret;
    while (true) {
        getALineState = GetALine();
        switch (getALineState) {
            case GET_A_LINE_CONTINUE: {
                return PARSE_REQUEST_RETURN_CODE_CONTINUE;
            }
            case GET_A_LINE_ERROR: {
                return PARSE_REQUEST_RETURN_CODE_ERROR;
            }
            case GET_A_LINE_OK: {
                ret = ParseSingleLine();
                if (ret == PARSE_REQUEST_RETURN_CODE_CONTINUE) {
                    break;
                } else {
                    return ret;
                }
            }
            default: {
                return PARSE_REQUEST_RETURN_CODE_ERROR;
            }
        }
    }

    return PARSE_REQUEST_RETURN_CODE_ERROR;
}

GetALineState HttpProcessor::GetALine()
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

ParseRequestReturnCode HttpProcessor::ParseSingleLine()
{
    switch (m_processState) {
        case HTTP_PROCESS_STATE_PARSE_REQUEST_LINE: {
            return ParseRequestLine();
        }
        case HTTP_PROCESS_STATE_PARSE_HEAD_FIELD: {
            return ParseHeadFields();
        }
        case HTTP_PROCESS_STATE_PARSE_MESSAGE_BODY: {
            return ParseContent();
        }
        default: {
            printf("ERROR Invalid state:%u.\n", m_processState);
            return PARSE_REQUEST_RETURN_CODE_ERROR;
        }
    }
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
    
    m_httpVersion = m_startPos;
    m_startPos += (strlen(m_startPos) + 1);
    if (strcasecmp(m_httpVersion, "HTTP/1.0") != 0) {
        printf("ERROR Support HTTP/1.0 only.\n");
        return PARSE_REQUEST_RETURN_CODE_ERROR;
    }

    m_processState = HTTP_PROCESS_STATE_PARSE_HEAD_FIELD;
    return PARSE_REQUEST_RETURN_CODE_CONTINUE;
}

bool HttpProcessor::GetFieldSplitedByWhiteSpaceChars(char *&field)
{
    char *ret = strpbrk(m_startPos, WHITE_SPACE_CHARS);
    if (ret == nullptr) {
        printf("ERROR Can't find white-space char: %s\n", m_startPos);
        return false;
    }
    *ret = '\0';
    field = m_startPos;
    ret++;
    m_startPos = ret + strspn(ret, whiteSpaceChars);
    return true;
}

ParseRequestReturnCode HttpProcessor::ParseHeadFields()
{
    if (m_startPos == END_CHAR) {
        if (m_contentLen != 0) {
            m_processState = HTTP_PROCESS_STATE_PARSE_MESSAGE_BODY;
            return PARSE_REQUEST_RETURN_CODE_CONTINUE;
        }
        return PARSE_REQUEST_RETURN_CODE_FINISH;
    }

    for (unsigned int i = 0; i < KEY_NAME_AND_PARSE_FUNC_MAP_LIST_SIZE; ++i) {
        KeyNameAndParseFuncMap map = KEY_NAME_AND_PARSE_FUNC_MAP_LIST[i];
        if (strncasecmp(m_startPos, map.keyName, strlen(map.keyName)) == 0) {
            m_startPos += strlen(map.keyName);
            if (m_startPos != HEAD_FIELD_SPLIT_CHAR) {
                printf("ERROR invalid head field:%s.\n", m_startPos);
                return PARSE_REQUEST_RETURN_CODE_ERROR;
            }
            m_startPos += 1; // 跳过':'
            m_startPos += strspn(m_startPos, "\t ");
            map.parseFunc();
            m_startPos += (strlen(m_startPos) + 1);
            return PARSE_REQUEST_RETURN_CODE_CONTINUE;
        }
    }

    m_startPos += (strlen(m_startPos) + 1);
    return PARSE_REQUEST_RETURN_CODE_CONTINUE;
}

void HttpProcessor::ParseContentLength()
{
    m_contentLen = atol(m_startPos);
}

void HttpProcessor::ParseConnection()
{
    if (strcasecmp(m_startPos, KEEP_ALIVE_VALUE) == 0) {
        m_keepAlive = true;
    }
}

ParseRequestReturnCode HttpProcessor::ParseContent()
{
    unsigned int parseSize = m_startPos - m_request; // 消息体前面信息所占字节数
    if (m_contentLen > MAX_READ_BUFF_LEN - parseSize) {
        printf("ERROR invalid Content-Length, m_contentLen = %u, parseSize = %u\n",
            m_contentLen, parseSize);
        return PARSE_REQUEST_RETURN_CODE_ERROR;
    }

    if (m_currentRequestSize - parseSize >= m_contentLen) {
        m_startPos[m_contentLen] = '\0';
        return PARSE_REQUEST_RETURN_CODE_FINISH;
    }

    return PARSE_REQUEST_RETURN_CODE_CONTINUE;
}

bool HttpProcessor::Response(const ParseRequestReturnCode returnCode)
{
    switch (returnCode) {
        case PARSE_REQUEST_RETURN_CODE_FINISH: {
            ResponseStatusCode statusCode = HandleRequest();
            break;
        }
        case PARSE_REQUEST_RETURN_CODE_ERROR: {
            break;
        }
        case ARSE_REQUEST_RETURN_CODE_CONTINUE: {
            return true;
        }
        default: {
            break;
        }
    }
    return false;
}

ResponseStatusCode HttpProcessor::HandleRequest()
{
    char filePath[MAX_FILE_NAME_LEN] = { 0 };
    int ret = sprintf_s(filePath, MAX_FILE_NAME_LEN, "%s%s", m_sourceDir, m_url);
    if (ret == -1) {
       printf("ERROR Get file path fail, dir:%s, url:%s.\n", m_sourceDir, m_url);
       return RESPONSE_STATUS_CODE_INTERNAL_SERVER_ERROR; 
    }

    struct stat fileStat{ 0 };
    if (stat(filePath, &fileStat) == -1) {
       printf("ERROR Get file stat fail, path:%s.\n", filePath);
       return RESPONSE_STATUS_CODE_NOT_FOUND;        
    }

    if ((fileStat.st_mode & S_IROTH) == 0) {
        printf("ERROR Can't read %s.\n", filePath);
        return RESPONSE_STATUS_CODE_FORBIDDEN;
    }

    if (S_ISDIR(fileStat.st_mode)) {
        printf("ERROR %s is dir.\n", filePath);
        return RESPONSE_STATUS_CODE_BAD_REQUEST;
    }

    int fd = open(filePath, O_RDONLY);
    if (fd == -1) {
        printf("ERROR open file fail: %s.\n", filePath);
        return RESPONSE_STATUS_CODE_INTERNAL_SERVER_ERROR; 
    }

    m_fileAddr = reinterpret_cast<char *>mmap(0, fileStat, PORT_READ, MAP_PRIVATE, fd, 0);
    m_fileSize = fileStat.st_size;
    close(fd);
    return RESPONSE_STATUS_CODE_OK;
}

bool HttpProcessor::AddResponse(const ResponseStatusCode statusCode)
{
    for (unsigned int i = 0; i < ERROR_STATUS_INFO_LIST_SIZE; ++i) {
        if (statusCode == RESPONSE_STATUS_CODE_OK) {
            return AddResponseInNormalCase();
        }
        if (statusCode == ERROR_STATUS_INFO_LIST[i].statusCode) {
            return AddResponseInErrorCase(ERROR_STATUS_INFO_LIST[i]);
        }
    }

    printf("ERROR Invalid statusCode: %u.\n", statusCode);
    return false;
}
bool HttpProcessor::AddResponseInNormalCase()
{
    if (!AddStatusLine(RESPONSE_STATUS_CODE_OK, OK_TITLE)) {
        return false;
    }
    if (!AddHeadField(strlen(OK_TITLE))) {
        return false;
    }

    m_iov[0].iov_base = m_writeBuff;
    m_iov[0].iov_len = m_writeSize;
    m_iov[1].iov_base = m_fileAddr;
    m_iov[1].iov_len = m_fileSize;
    m_cnt = 2;
    return true;
}

bool HttpProcessor::AddResponseInErrorCase(const StatusInfo statusInfo)
{
    if (!AddStatusLine(statusInfo.statusCode, statusInfo.statusTitle)) {
        return false;
    }
    if (!AddHeadField(strlen(statusInfo.statusTitle))) {
        return false;
    }
    if (!AddContent(statusInfo.statusContent)) {
        return false;
    }

    m_iov[0].iov_base = m_writeBuff;
    m_iov[0].iov_len = m_writeSize;
    m_cnt = 1;
    return true;
}

bool HttpProcessor::AddStatusLine(const int status, const char *title)
{
    if (m_writeSize >= MAX_WRITE_BUFF_LEN) {
        printf("ERROR Write buffer is full, m_writeSize = %u\n", m_writeSize);
        return false;
    }
    int ret = sprintf_s(m_writeBuff, MAX_WRITE_BUFF_LEN - m_writeSize, "%s %d %s\r\n",
        HTTP_VERSION, status, title);
    if (ret == -1) {
        printf("ERROR Write buffer fail.\n");
        return false;
    }
    m_writeSize += static_cast<unsigned int>(ret);

    return true;
}

bool HttpProcessor::AddHeadField(const unsigned int contentLen)
{
    if (m_writeSize >= MAX_WRITE_BUFF_LEN) {
        printf("ERROR Write buffer is full, m_writeSize = %u\n", m_writeSize);
        return false;
    }
    // 添加消息体长度
    int ret = sprintf_s(m_writeBuff, MAX_WRITE_BUFF_LEN - m_writeSize,
        "Content-Length: %d\r\n", contentLen);
    if (ret == -1) {
        printf("ERROR Write buffer fail.\n");
        return false;
    }
    m_writeSize += static_cast<unsigned int>(ret);
    // 添加连接方式
    ret = sprintf_s(m_writeBuff, MAX_WRITE_BUFF_LEN - m_writeSize,
        "Connection: %s\r\n", m_keepAlive ? KEEP_ALIVE_VALUE : CLOSE_ALIVE_VALUE);
    if (ret == -1) {
        printf("ERROR Write buffer fail.\n");
        return false;
    }
    m_writeSize += static_cast<unsigned int>(ret);
    // 添加空行
    ret = sprintf_s(m_writeBuff, MAX_WRITE_BUFF_LEN - m_writeSize, "\r\n");
    if (ret == -1) {
        printf("ERROR Write buffer fail.\n");
        return false;
    }
    m_writeSize += static_cast<unsigned int>(ret);
    return true;
}

bool HttpProcessor::AddContent(const char *content)
{
    if (m_writeSize >= MAX_WRITE_BUFF_LEN) {
        printf("ERROR Write buffer is full, m_writeSize = %u\n", m_writeSize);
        return false;
    }

    int ret = sprintf_s(m_writeBuff, MAX_WRITE_BUFF_LEN - m_writeSize, "%s\r\n",
        content);
    if (ret == -1) {
        printf("ERROR Write buffer fail.\n");
        return false;
    }
    m_writeSize += static_cast<unsigned int>(ret);
    return true;
}
