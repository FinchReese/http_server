#include "thread_pool.h"

ThreadPool::ThreadPool(const unsigned int poolNum) : m_threadNum(poolNum),
    m_threadPool(nullptr), m_stop(true)
{}

ThreadPool::~ThreadPool()
{
    // 销毁互斥锁
    (void)pthread_mutex_destroy(&m_mutex);
    // 销毁信号量
    (void)sem_destropy(&m_sem);
    // 销毁线程池
    m_stop = true;
    if (m_threadPool != nullptr) {
        delete []m_threadPool;
    }
}

bool ThreadPool::Init()
{
    // 初始化互斥锁
    int ret = pthread_mutex_init(&m_mutex, nullptr);
    if (ret != 0) {
        printf("ERROR pthread_mutex_init fail, ret = %d\n", ret);
        return false;
    }
    // 初始化信号量
    int ret = sem_init(&m_sem, 0, 0);
    if (ret != 0) {
        printf("ERROR sem_init fail, ret = %d\n", ret);
        return false;
    }
    // 初始化线程池
    m_threadPool = new thread_t[m_threadNum];
    if (m_threadPool == nullptr) {
        return false;
    }

    for (unsigned int i = 0; i < m_threadNum; ++i) {
        if(pthread_create(&m_threadPool[i], nullptr, ThreadPool::ThreadFunction, this) != 0) {
            delete []m_threadPool;
            m_threadPool = nullptr;
            return false;
        }
        if (pthread_detach() != 0) {
            delete []m_threadPool;
            m_threadPool = nullptr;
            return false;            
        }
    }


    return true;
}

bool ThreadPool::AddTask(const Task &task)
{
    if (pthread_mutex_lock(&m_mutex) != 0) {
        return false;
    }

    if (m_queue.size() == m_queue.max_size()) {
        (void)pthread_mutex_unlock(&m_mutex);
        return false;
    }
    m_queue.push(task);
    (void)pthread_mutex_unlock(&m_mutex);

    if (sem_post(&m_sem) != 0) {
        return false;
    }

    return true;
}

void *ThreadPool::ThreadFunction(void *argv)
{
    ThreadPool *pool = reinterpret_cast<ThreadPool *>(argv);
    pool->Run();
    return pool;
}

void *ThreadPool::Run(void *argv)
{
    while (m_stop == false) {
        if (sem_wait(&m_sem) != 0) {
            continue;
        }
        if (pthread_mutex_lock(&m_mutex) != 0) {
            continue;
        }

        if (m_queue.empty()) {
            (void)pthread_mutex_unlock(&m_mutex);
            continue;
        }

        Task task = m_queue.front();
        m_queue.pop();
        (void)pthread_mutex_unlock(&m_mutex);
        task.function(task.argv);
    }

}
