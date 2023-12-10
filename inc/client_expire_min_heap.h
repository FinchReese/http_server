#ifndef CLIENT_EXPIRE_MIN_HEAP_H
#define CLIENT_EXPIRE_MIN_HEAP_H

#include <time.h>

typedef struct {
    int clientFd;
    time_t expire;
} ClientExpire;

const unsigned int MAX_SOCKET_FD = 0xFFFF; // 套接字最大值是0xFFFF
const unsigned int MAX_SOCKET_NUM = MAX_SOCKET_FD + 1; // 套接字最大数量等于套接字最大值+1

class ClientExpireMinHeap {
public:
    ClientExpireMinHeap();
    ~ClientExpireMinHeap();
    bool Init(unsigned int heapSize);
    bool Init(unsigned int capacity, ClientExpire *array, const unsigned int arraySize);
    bool push(const ClientExpire &node);
    bool pop(ClientExpire &node);
    bool top(ClientExpire &node);
private:
    void siftDown(const unsigned int startIdx);
    void siftUp(const unsigned int startIdx);
    bool resize();
private:
    unsigned int m_capacity;
    unsigned int m_currentSize;
    ClientExpire *m_heap;
    unsigned int m_clientIdxList[MAX_SOCKET_NUM]; // 记录客户端socket在m_heap的下标

};
#endif