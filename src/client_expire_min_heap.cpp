#include "client_expire_min_heap.h"

#include "min_heap.h"

const unsigned int MAX_U32 = 0xFFFFFFFF; // 最大的U32

ClientExpireMinHeap::ClientExpireMinHeap() : m_capacity(0), m_currentSize(0), m_heap(nullptr)
{}

ClientExpireMinHeap::~ClientExpireMinHeap()
{
    if (m_heap != nullptr) {
        delete []m_heap;
        m_heap = nullptr;
    }
}

bool ClientExpireMinHeap::Init(unsigned int capacity)
{
    if (m_heap != nullptr) {
        delete []m_heap;
        m_heap = nullptr;
    }

    m_heap = new ClientExpire[capacity];
    if (m_heap == nullptr) {
        m_capacity = 0;
        return false;
    }

    m_capacity = capacity;
    m_currentSize = 0;
    return true;
}

bool ClientExpireMinHeap::Init(unsigned int capacity, ClientExpire *array, const unsigned int arraySize)
{
    // 清理最小堆
    if (m_heap != nullptr) {
        m_currentSize = 0;
        delete []m_heap;
        m_heap = nullptr;
    }
    // 申请最小堆空间
    m_heap = new ClientExpire[capacity];
    if (m_heap == nullptr) {
        m_capacity = 0;
        return false;
    }
    m_capacity = capacity;
    m_currentSize = 0;
    // 初始化最小堆
    if (array != nullptr) {
        for (unsigned int i = 0; i < arraySize; ++i) {
            if (push(arraySize[i]) == false) {
                return false;
            }
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
            if (m_heap[childIdx + 1].m_expire < m_heap[childIdx].m_expire) {
                childIdx++;
            }
        }
        if (m_heap[childIdx].m_expire >= value.m_expire) {
            break;
        }
        m_heap[currentIdx] = m_heap[childIdx];
        currentIdx = childIdx;
        childIdx = currentIdx * 2 + 1; 
    }
    m_heap[currentIdx] = value;
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
        if (m_heap[parentIdx].m_expire <= value.m_expire) {
            break;
        }
        m_heap[currentIdx] = m_heap[parentIdx];
        currentIdx = parentIdx;
    }
    m_heap[currentIdx] = value;
}

bool ClientExpireMinHeap::resize()
{
    unsigned int newCapacity = m_capacity * 2;
    if (m_capacity > MAX_U32 / 2) {
        newCapacity = MAX_U32;
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
    
    m_heap[m_currentSize++] = node;
    siftUp(m_currentSize - 1);
    return true;
}

bool ClientExpireMinHeap::pop(ClientExpire &node)
{
    if (m_currentSize == 0) {
        return false;
    }

    node = m_head[0];
    m_head[0] = m_head[m_currentSize - 1];
    m_currentSize--;
    siftDown(0);
    return true;
}

bool ClientExpireMinHeap::top(ClientExpire &node)
{
    if (m_currentSize == 0) {
        return false;
    }

    node = m_head[0];
    return true;
}

bool ClientExpireMinHeap::modify(const ClientExpire &node)
{
}