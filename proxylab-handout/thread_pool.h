#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <pthread.h>

#ifndef NULL
#define NULL 0
#endif

typedef struct _task {
    void *(*process)(void* arg);
    void *arg;
    struct _task* next;
} task;

struct threadinfo {
    int     thread_running;     //线程退出
    int     thread_num;
    int     task_num;
    task*           tasks;
    pthread_t*      thread_id;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
};

/* 初始化线程池线程数目
 * @param thread_num
 */
void init_thread_pool(int thread_num);

/* 销毁线程池
 */
void destroy_thread_pool();

/* 向线程池增加一个任务
 * @param process   执行任务函数
 * @param arg       执行任务函数参数
 */
void thread_pool_add_task(void *(*process)(void* arg), void *arg);

/* 从线程池取一个任务
 * @return 返回得到的任务
 */
task* thread_pool_retrieve_task();

/* 线程函数
 * @param thread_param 线程参数
 */
void* thread_routine(void* thread_param);

#endif