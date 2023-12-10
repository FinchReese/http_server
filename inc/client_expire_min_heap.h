#ifndef CLIENT_EXPIRE_MIN_HEAP_H
#define CLIENT_EXPIRE_MIN_HEAP_H

#include <time.h>

typedef struct {
    int m_clientFd;
    time_t m_expire;
} ClientExpire;

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

};
#endif