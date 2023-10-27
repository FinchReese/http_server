#ifndef CLIENT_EXPIRE_H
#define CLIENT_EXPIRE_H
#include <time.h>

class ClientExpire {
public:
    ClientExpire(const int fd, const time_t expire) : m_clientFd(fd), m_expire(expire) {}
    ClientExpire(const ClientExpire &clientExpire) : m_clientFd(clientExpire.m_clientFd), m_expire(clientExpire.m_expire) {}
    ~ClientExpire();
    bool operator<(const ClientExpire &cmpClientExpire);
    bool operator<=(const ClientExpire &cmpClientExpire);
    bool operator>(const ClientExpire &cmpClientExpire);
    bool operator>=(const ClientExpire &cmpClientExpire);
    ClientExpire &operator=(const ClientExpire &clientExpire);
public:
    int m_clientFd;
    time_t m_expire;
};

#endif