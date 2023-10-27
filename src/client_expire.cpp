#include "client_expire.h"

bool ClientExpire::operator<(const ClientExpire &cmpClientExpire)
{
    return m_expire < cmpClientExpire.m_expire;
}

bool ClientExpire::operator<=(const ClientExpire &cmpClientExpire)
{
    return m_expire <= cmpClientExpire.m_expire;
}

bool ClientExpire::operator>(const ClientExpire &cmpClientExpire)
{
    return m_expire > cmpClientExpire.m_expire;
}

bool ClientExpire::operator>=(const ClientExpire &cmpClientExpire)
{
    return m_expire >= cmpClientExpire.m_expire;
}

ClientExpire &ClientExpire::operator=(const ClientExpire &clientExpire)
{
    if (this == &clientExpire) {
        return *this;
    }

    m_clientFd = clientExpire.m_clientFd;
    m_expire = clientExpire.m_expire;
    return *this;
}