#include "thread_pool.h"

#include <stdio.h>
#include <stdlib.h>

struct threadinfo g_threadinfo;

/* 初始化线程池线程数目
 * @param thread_num
 */
void init_thread_pool(int thread_num) {
    if (thread_num <= 0) thread_num = 8;

    pthread_mutex_init(&g_threadinfo.mutex, NULL);
    pthread_cond_init(&g_threadinfo.cond, NULL);

    g_threadinfo.thread_num = thread_num;
    g_threadinfo.task_num = 0;
    g_threadinfo.tasks = NULL;
    g_threadinfo.thread_running = 1;
    
    g_threadinfo.thread_id = (pthread_t *) malloc(sizeof(pthread_t) * thread_num);
    int i = 0;
    for (; i < thread_num; i++) {
        pthread_create(&g_threadinfo.thread_id[i], NULL, thread_routine, NULL);
    }
}

/* 销毁线程池
 */
void destroy_thread_pool() {
    g_threadinfo.thread_running = 0;
    pthread_cond_broadcast(&g_threadinfo.cond);

    int i = 0;
    for (; i < g_threadinfo.thread_num; i++) {
        pthread_join(g_threadinfo.thread_id[i], NULL);
    }

    free(g_threadinfo.thread_id);

    task* head = NULL;
    while(g_threadinfo.tasks != NULL) {
        head = g_threadinfo.tasks;
        g_threadinfo.tasks = head->next;
        free(head);
    }

    pthread_mutex_destroy(&g_threadinfo.mutex);
    pthread_cond_destroy(&g_threadinfo.cond);
}

/* 向线程池增加一个任务
 * @param t 需要增加的任务
 */
void thread_pool_add_task(void *(*process)(void* arg), void *arg) {
    if (process == NULL) return;

    task* t = (task*) malloc(sizeof(task));
    t->process = process;
    t->arg = arg;
    t->next = NULL;
    pthread_mutex_lock(&g_threadinfo.mutex);
    task* head = g_threadinfo.tasks;
    if (head == NULL) {
        head = t;
    } else {
        while (head->next != NULL) {
            head = head->next;
        }
        head->next = t;
    }
    g_threadinfo.task_num++;
    pthread_mutex_unlock(&g_threadinfo.mutex);
    pthread_cond_signal(&g_threadinfo.cond);
}

/* 从线程池取一个任务
 * @return 返回得到的任务
 */
task* thread_pool_retrieve_task() {
    task* head = g_threadinfo.tasks;
    if (head != NULL) {
        g_threadinfo.tasks = head->next;
        g_threadinfo.task_num--;
        return head;
    }
    return NULL;
}

/* 线程函数
 * @param thread_param 线程参数
 */
void* thread_routine(void* thread_param) {
    while(g_threadinfo.thread_running) {
        pthread_mutex_lock(&g_threadinfo.mutex);
        // 队列为空，没有任务
        while(g_threadinfo.task_num <= 0) {
            pthread_cond_wait(&g_threadinfo.cond, &g_threadinfo.mutex);
            if (!g_threadinfo.thread_running) break;
        }
        task* t = thread_pool_retrieve_task();
        pthread_mutex_unlock(&g_threadinfo.mutex);
        
        // 执行任务
        if (t != NULL) {
            (*(t->process)) (t->arg);
            free(t);
        }
    }
}