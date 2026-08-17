/**
 * 按不同顺序访问互斥锁导致死锁
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

int a = 0;
int b = 0;

// 互斥锁
pthread_mutex_t mutex_a;
pthread_mutex_t mutex_b;

void *another(void *arg) {
    // 加锁
    pthread_mutex_lock(&mutex_b);
    printf("in child thread, got mutex b, waiting for mutex a\n");
    sleep(5);
    ++b;

    // 加锁
    pthread_mutex_lock(&mutex_a);
    b += a++;
    // 解锁
    pthread_mutex_unlock(&mutex_a);
    pthread_mutex_unlock(&mutex_b);
    // 退出线程
    pthread_exit(NULL);
}

int main() {
    pthread_t id;
    pthread_mutex_init(&mutex_a, NULL);
    pthread_mutex_init(&mutex_b, NULL);

    // 创建线程
    pthread_create(&id, NULL, another, NULL);

    pthread_mutex_lock(&mutex_a);
    printf("in parent thread, got mutex a, waiting for mutex b\n");
    sleep(5);
    ++a;
    pthread_mutex_lock(&mutex_b);
    a += b++;
    pthread_mutex_unlock(&mutex_b);
    pthread_mutex_unlock(&mutex_a);

    pthread_join(id, NULL); // 等待子线程结束
    pthread_mutex_destroy(&mutex_a);
    pthread_mutex_destroy(&mutex_b);

    return 0;
}