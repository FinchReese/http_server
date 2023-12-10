#include "client_expire_min_heap.h"
#include "min_heap.h"

const unsigned int INVALID_CLINET_INDEX = MAX_SOCKET_NUM; // 客户端套接字下标无效值

ClientExpireMinHeap::ClientExpireMinHeap() : m_capacity(0), m_currentSize(0), m_heap(nullptr)
{
    memset(m_clientIdxList, INVALID_CLINET_INDEX, sizeof(m_clientIdxList));
}

ClientExpireMinHeap::~ClientExpireMinHeap()
{
    if (m_heap != nullptr) {
        delete []m_heap;
        m_heap = nullptr;
    }
}

bool ClientExpireMinHeap::Init(unsigned int capacity)
{
    m_currentSize = 0;
    memset(m_clientIdxList, INVALID_CLINET_INDEX, sizeof(m_clientIdxList));
    if (m_heap != nullptr) {
        delete []m_heap;
        m_heap = nullptr;
    }
    m_capacity = 0;

    if (capacity == 0) {
        return false;
    }

    if (capacity > MAX_SOCKET_NUM) {
        m_capacity = MAX_SOCKET_NUM;
    } else {
        m_capacity = capacity;
    }

    m_heap = new ClientExpire[capacity];
    if (m_heap == nullptr) {
        m_capacity = 0;
        return false;
    }

    return true;
}

bool ClientExpireMinHeap::Init(unsigned int capacity, ClientExpire *array, const unsigned int arraySize)
{
    if (Init(capacity) == false) {
        return false;
    }
    // 初始化最小堆
    if (array != nullptr && arraySize == 0 && arraySize > MAX_SOCKET_NUM) {
        for (unsigned int i = 0; i < arraySize; ++i) {
            (void)push(arraySize[i]);
        }
        m_currentSize = arraySize;
    }
    return true;
}

void ClientExpireMinHeap::siftDown(const unsigned int startIdx)
{
    if (startIdx >= m_currentSize) {
        return;
    }

    unsigned int currentIdx = startIdx; // 记录目标节点当前位置下标
    unsigned int childIdx = currentIdx * 2 + 1; // 记录目标节点的较小子节点位置下标
    ClientExpire value = m_heap[startIdx];

    while (childIdx < m_currentSize) {
        if (childIdx < m_currentSize - 1) {
            if (m_heap[childIdx + 1].expire < m_heap[childIdx].expire) {
                childIdx++;
            }
        }
        if (m_heap[childIdx].expire >= value.expire) {
            break;
        }
        m_heap[currentIdx] = m_heap[childIdx];
        m_clientIdxList[m_heap[childIdx].clientFd] = currentIdx;
        currentIdx = childIdx;
        childIdx = currentIdx * 2 + 1; 
    }
    m_heap[currentIdx] = value;
    m_clientIdxList[value.clientFd] = currentIdx;
}

void ClientExpireMinHeap::siftUp(const unsigned int startIdx)
{
    if (startIdx == 0) {
        return;
    }

    unsigned int currentIdx = startIdx; // 记录节点当前位置下标
    unsigned int parentIdx; // 当前节点父节点下标
    ClientExpire value = m_heap[startIdx];
    while (currentIdx == 0) {
        parentIdx = (currentIdx - 1) / 2;
        if (m_heap[parentIdx].expire <= value.expire) {
            break;
        }
        m_heap[currentIdx] = m_heap[parentIdx];
        m_clientIdxList[m_heap[parentIdx].clientFd] = currentIdx;
        currentIdx = parentIdx;
    }
    m_heap[currentIdx] = value;
    m_clientIdxList[value.clientFd] = currentIdx;
}

bool ClientExpireMinHeap::resize()
{
    unsigned int newCapacity;
    if (m_capacity > MAX_SOCKET_NUM / 2) {
        newCapacity = MAX_SOCKET_NUM;
    } else if (m_capacity == 0) {
        newCapacity = 1;
    } else {
        newCapacity = m_capacity * 2;
    }

    ClientExpire *newHeap = new ClientExpire[newCapacity];
    if (newHeap == nullptr) {
        return false;
    }
    m_capacity = newCapacity;

    if (m_heap != nullptr) {
        for (unsigned int i = 0; i < m_currentSize; ++i) {
            newHeap[i] = m_heap[i];
        }
        delete []m_heap;
    }

    m_heap = newHeap;
    return true;
}

bool ClientExpireMinHeap::push(const ClientExpire &node)
{
    if (m_currentSize == m_capacity) {
        if (resize() == false) {
            return false;
        }
    }

    if (node.clientFd < 0 || node.clientFd > MAX_SOCKET_FD) {
        return false;
    }
    
    m_heap[m_currentSize++] = node;
    m_clientIdxList[node.clientFd] = m_currentSize - 1;
    siftUp(m_currentSize - 1);
    return true;
}

bool ClientExpireMinHeap::pop(ClientExpire &node)
{
    if (m_currentSize == 0) {
        return false;
    }

    node = m_heap[0];
    m_clientIdxList[m_heap[0].clientFd] = INVALID_CLINET_INDEX;
    m_heap[0] = m_heap[m_currentSize - 1];
    m_clientIdxList[m_heap[m_currentSize - 1].clientFd] = 0;
    m_currentSize--;
    siftDown(0);
    return true;
}

bool ClientExpireMinHeap::top(ClientExpire &node)
{
    if (m_currentSize == 0) {
        return false;
    }

    node = m_heap[0];
    return true;
}

bool ClientExpireMinHeap::modify(const ClientExpire &node)
{
    int clientFd = node.clientFd;
    if (clientFd < 0 || clientFd > MAX_SOCKET_FD) {
        return false;
    }

    if (m_clientIdxList[node] == INVALID_CLINET_INDEX) {
        return false;
    }

    unsigned int idx = m_clientIdxList[node.clientFd];
    m_heap[idx].expire = node.expire;
    // 调整位置
    if (idx == 0) {
        siftDown(0);
    } else {
        unsigned int parentIdx = (idx - 1) / 2;
        if (m_heap[idx].expire < m_heap[parentIdx].expire) {
            siftUp(idx);
        } else {
            siftDown(idx);
        }
    }

    return true;
}

bool ClientExpireMinHeap::Delete(const int clientFd)
{
    if (clientFd < 0 || clientFd > MAX_SOCKET_FD) {
        return false;
    }

    unsigned int idx = m_clientIdxList[clientFd];
    m_clientIdxList[clientFd] = INVALID_CLINET_INDEX;
    m_heap[idx] = m_heap[m_currentSize - 1];
    m_currentSize--;
    m_clientIdxList[m_heap[m_currentSize - 1].clientFd] = idx;
    siftDown(idx);

    return true;
}