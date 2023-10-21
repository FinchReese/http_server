#include "min_heap.h"

const unsigned int MAX_U32 = 0xFFFFFFFF; // 最大的U32

template <class T>
MinHeap<T>::MinHeap() : m_capacity(0), m_currentSize(0), m_heap(nullptr)
{}

template <class T>
MinHeap<T>::~MinHeap()
{
    if (m_heap != nullptr) {
        delete []m_heap;
        m_heap = nullptr;
    }
}

template <class T>
bool MinHeap<T>::Init(unsigned int capacity)
{
    if (m_heap != nullptr) {
        delete []m_heap;
        m_heap = nullptr;
    }

    m_heap = new T[capacity];
    if (m_heap == nullptr) {
        m_capacity = 0;
        return false;
    }

    m_capacity = capacity;
    m_currentSize = 0;
    return true;
}

template <class T>
bool MinHeap<T>::Init(unsigned int capacity, T *array, const unsigned int arraySize)
{
    if (m_heap != nullptr) {
        m_currentSize = 0;
        delete []m_heap;
        m_heap = nullptr;
    }

    m_heap = new T[capacity];
    if (m_heap == nullptr) {
        m_capacity = 0;
        return false;
    }
    m_capacity = capacity;
    if (array != nullptr) {
        m_currentSize = arraySize;
        for (unsigned int i = 0; i < arraySize; ++i) {
            m_heap[i] = array[i];
        }
    }
    return true;
}

template<class T>
void MinHeap<T>::siftDown(const unsigned int startIdx)
{
    if (startIdx >= m_currentSize) {
        return;
    }

    if (startIdx > (MAX_U32 -1) / 2) { // 说明当前节点是叶子节点，不继续处理
        return;
    }

    unsigned int currentIdx = startIdx; // 记录目标节点当前位置下标
    unsigned int childIdx = currentIdx * 2 + 1; // 记录目标节点的较小子节点位置下标
    T value = m_heap[startIdx];

    while (childIdx < m_currentSize) {
        if (childIdx < m_currentSize - 1) {
            if (m_heap[childIdx + 1] < m_heap[childIdx]) {
                childIdx++;
            }
        }
        if (m_heap[childIdx] >= value) {
            break;
        }
        m_heap[currentIdx] = m_heap[childIdx];
        currentIdx = childIdx;
        if (currentIdx > (MAX_U32 -1) / 2) { // 说明当前节点是叶子节点，不继续处理
            return;
        }
        childIdx = currentIdx * 2 + 1; 
    }
    m_heap[currentIdx] = value;
}

template<class T>
void MinHeap<T>::siftUp(const unsigned int startIdx)
{
    if (startIdx == 0) {
        return;
    }

    unsigned int currentIdx = startIdx; // 记录节点当前位置下标
    unsigned int parentIdx = (currentIdx - 1) / 2; // 当前节点父节点下标
    T value = m_heap[startIdx];
    while (true) {
        if (m_heap[parentIdx] <= value) {
            break;
        }
        m_heap[currentIdx] = m_heap[parentIdx];
        currentIdx = parentIdx;
        if (currentIdx == 0) {
            break;
        }
        parentIdx = (currentIdx - 1) / 2;
    }
    m_heap[currentIdx] = value;
}

template<class T>
bool MinHeap<T>::resize()
{
    if (m_capacity > MAX_U32 / 2) {
        return false;
    }
    unsigned int newCapacity = m_capacity * 2;

    T *newHeap = new T[newCapacity];
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

template<class T>
bool MinHeap<T>::push(const T node)
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

template<class T>
bool MinHeap<T>::pop(const T &node)
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