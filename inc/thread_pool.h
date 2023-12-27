#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <semaphore.h>
#include <queue>

typedef void *(TaskFunction)(void *);

typedef struct {
    TaskFunction function;
    void *argv;
} Task;

class ThreadPool {
public:
    ThreadPool(const unsigned int threadNum);
    ~ThreadPool();
    bool Init();
    bool AddTask(const Task &task);
private:
    static void *ThreadFunction(void *argv);
    void *Run(void *argv);
private:
    unsigned int m_threadNum;
    pthread_t *m_threadPool;
    bool m_stop;
    std::queue<Task> m_queue;
    pthread_mutex_t m_mutex;
    sem_t m_sem;
};

#endif