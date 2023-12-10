#ifndef MIN_HEAP_H
#define MIN_HEAP_H

template <class T>
class MinHeap {
public:
    MinHeap();
    ~MinHeap();
    bool Init(unsigned int heapSize);
    bool Init(unsigned int capacity, T *array, const unsigned int arraySize);
    bool push(const T node);
    bool pop(const T &node);
    bool top(const T &node);
private:
    void siftDown(const unsigned int startIdx);
    void siftUp(const unsigned int startIdx);
    bool resize();
private:
    unsigned int m_capacity;
    unsigned int m_currentSize;
    T *m_heap;

};

#endif