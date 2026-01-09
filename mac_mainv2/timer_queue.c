#define _GNU_SOURCE
#include "timer_queue.h"
#include "cvector.h"
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include "time.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "cvector.h"
#define min(a, b) ((a > b) ? b : a)
#define max(a, b) ((a > b) ? a : b)
#define filename(x) strrchr(x, '/') ? strrchr(x, '/') + 1 : x
#define LOG(format, args...) (printf("[%s]-[%d]:" format "\r\n", filename(__FILE__), __LINE__, ##args))
volatile int queue_checking = 0; // 标志位
typedef struct
{
    char name[20];                // 定时器事件的名称
    uint8_t type;                 // 0表示一次性定时器，1表示周期定时器
    struct timeval delay_time;    // 延时高精度时间
    struct timeval out_time;      // 超时高精度时间
    void (*timeout_func)(void *); // 回调函数
    void *arg;                    // 回调函数参数
    int id;                       // 新增唯一标识符，用于查找和删除
} stimer_event;

typedef struct
{
    uint8_t start_flag;
    pthread_mutex_t lock;
    struct timeval next_outtime; // 超时高精度时间
    cvector queue;
} stimer_queue;

static stimer_queue stimer_queue_g;
int stimer_queue_remove(const char *timer_name)
{
    stimer_queue *ptr = &stimer_queue_g; // 获取全局定时器队列
    cvector *queue = &(ptr->queue);      // 定时器队列是用 cvector 维护的
    pthread_mutex_lock(&ptr->lock);      // 加锁以确保线程安全

    // 遍历定时器队列找到匹配的事件
    for (uint32_t i = 0; i < queue->size; i++)
    {
        struct link_list *current = cvector_llget(queue, i); // 获取第 i 个节点
        if (current == NULL || current->data == NULL)
        {
            LOG("[ERROR] Invalid node at index %d.", i);
            continue; // 如果节点无效，跳过
        }

        // 取出定时器事件
        stimer_event *event = (stimer_event *)(current->data);

        // 比较定时器名称，找到待删除的事件
        if (strcmp(event->name, timer_name) == 0)
        {
            LOG("Removing timer '%s' at index %d.", timer_name, i);

            // 从队列中删除该事件
            cvector_erase(queue, i);          // 使用 cvector 的删除接口
            pthread_mutex_unlock(&ptr->lock); // 解锁
            return CVESUCCESS;                // 成功删除
        }
    }

    LOG("[WARNING] Timer '%s' not found.", timer_name);
    pthread_mutex_unlock(&ptr->lock); // 如果未找到也要解锁
    return CVEFAILED;                 // 定时器未找到，返回失败
}
/* 有序定时器事件队列插入 */
void stimer_queue_order_insert(stimer_event *event)
{
    stimer_queue *ptr = &stimer_queue_g;
    cvector *queue = &(ptr->queue);
    struct link_list *llptr = queue->head;
    uint32_t i;
    // LOG("queue->size=%d", queue->size);
    /* 从前往后比较超时时刻 */
    for (i = 0; i < queue->size; i++)
    {
        stimer_event *tmp_event = (stimer_event *)llptr->data;
        if (timercmp(&(event->out_time), &(tmp_event->out_time), <))
        {
            //            LOG("next timeout:%lld sec %ld usec", event->out_time.tv_sec, event->out_time.tv_usec);
            //            LOG("next timeout:%lld sec %ld usec", tmp_event->out_time.tv_sec, tmp_event->out_time.tv_usec);
            break;
        }
        llptr = llptr->next;
    }
    // LOG("push %d/%d", i, queue->size);
    cvector_insert(queue, i, (char *)event, sizeof(stimer_event));
}

int32_t stimer_queue_start();

void stimer_queue_check()
{
    stimer_queue *ptr = &stimer_queue_g;
    cvector *queue = &(ptr->queue);
    pthread_mutex_lock(&ptr->lock);
    //    LOG("in check");
    queue_checking = 1; // 设置标志位
    // 处理所有超时的回调函数
    while (queue->head != NULL)
    {
        /* 获取当前高精度时间 */
        struct timeval cur_time;
        gettimeofday(&cur_time, NULL);
        /* 遍历有序数组 */
        stimer_event *event = (stimer_event *)queue->head->data;
        if (timercmp(&(event->out_time), &cur_time, <=))
        {
            /* 执行回调 */
            event->timeout_func(event->arg);

            /* 周期定时器重新插入 */
            if (event->type != 0)
            {
                //                LOG("cur sec=%lld, usec=%ld", event->out_time.tv_sec, event->out_time.tv_usec);
                timeradd(&event->out_time, &event->delay_time, &event->out_time);
                stimer_queue_order_insert(event);
            }
            // LOG("pop tick:%lld", event->timeout_tick);
            cvector_pophead(queue, NULL, 0);
        }
        else
        {
            break;
        }
    }
    queue_checking = 0; // 设置标志位
    /* 将定时器拨到下个事件 */
    stimer_queue_start();

    pthread_mutex_unlock(&ptr->lock);
}

int32_t stimer_queue_start()
{
    struct timeval cur_time = {0};
    gettimeofday(&cur_time, NULL);
    stimer_queue *ptr = &stimer_queue_g;
    /* 若队列事件已经处理完，直接返回 */
    if (ptr->queue.size == 0)
    {
        ptr->start_flag = 0;
        return 0;
    }

    stimer_event *event = (stimer_event *)ptr->queue.head->data;

    /* 启动定时器 */
    struct sigaction act;
    act.sa_flags = 0;
    act.sa_handler = stimer_queue_check;
    sigemptyset(&act.sa_mask);
    sigaction(SIGALRM, &act, NULL);
    // 设置定时器
    struct itimerval val = {0};
    //    val.it_value = event->delay_time;
    if (timercmp(&event->out_time, &cur_time, <))
    {
        val.it_value.tv_sec = 0;
        val.it_value.tv_usec = 1;
    }
    else
    {
        timersub(&event->out_time, &cur_time, &val.it_value);
    }
    setitimer(ITIMER_REAL, &val, NULL);
    ptr->next_outtime = event->out_time;
    //    LOG("next timeout:%lld sec %ld usec", val.it_value.tv_sec, val.it_value.tv_usec);
    //    LOG("next timeout:%lld sec %ld usec", ptr->next_outtime.tv_sec, ptr->next_outtime.tv_usec);

    return 0;
}

/* 以毫秒为单位的定时器队列添加函数 */
int stimer_queue_add(
    const char *timer_name,
    void (*timeout_func)(void *),
    void *arg,
    uint32_t type,
    uint32_t period_ms)
{
    stimer_event event = {0};
    /* 记录当前时间 */
    gettimeofday(&event.out_time, NULL);
    memcpy(event.name, timer_name, min(strlen(timer_name), sizeof(event.name)));
    event.timeout_func = timeout_func;
    event.arg = arg;
    event.type = type;
    //    LOG("delay %dms", period_ms);
    //    LOG("cur sec=%lld, usec=%ld", event.out_time.tv_sec, event.out_time.tv_usec);
    event.delay_time.tv_sec = period_ms / 1000;
    event.delay_time.tv_usec = (period_ms * 1000) % 1000000;
    //    LOG("sec=%lld, usec=%ld", event.delay_time.tv_sec, event.delay_time.tv_usec);
    /* 记录超时时间 */
    timeradd(&event.out_time, &event.delay_time, &event.out_time);

    stimer_queue *ptr = &stimer_queue_g;
    pthread_mutex_lock(&ptr->lock);
    /* 插入有序队列 */
    stimer_queue_order_insert(&event);

    /* 检查队列并启动定时器 */
    /* 定时器没有开启时开启 */
    if (ptr->start_flag == 0)
    {
        LOG("first start");
        ptr->start_flag = 1;
        stimer_queue_start();
        goto end;
    }

    /* 若新加入的事件是最早触发的，需要暂停当前定时任务，重新开启队列头事件定时任务 */
    if (timercmp(&(event.out_time), &(ptr->next_outtime), <))
    {
        //        LOG("next timeout:%lld sec %ld usec", event.out_time.tv_sec, event.out_time.tv_usec);
        //        LOG("next timeout:%lld sec %ld usec", ptr->next_outtime.tv_sec, ptr->next_outtime.tv_usec);
        LOG("cut in line");
        stimer_queue_start();
        goto end;
    }
end:
    pthread_mutex_unlock(&ptr->lock);

    // LOG("%s timer push done.", timer_name);
}

/* 以微秒为单位的定时器队列添加函数 */
int stimer_queue_add_us(const char *timer_name, void (*timeout_func)(void *), void *arg, uint32_t type, uint32_t period_us)
{
    printf("Step 1: Started stimer_queue_add_us\n");

    stimer_event event = {0};
    printf("Step 2: Event initialized.\n");

    gettimeofday(&event.out_time, NULL);
    printf("Step 3: Current time recorded.\n");

    memcpy(event.name, timer_name, min(strlen(timer_name), sizeof(event.name) - 1));
    printf("Step 4: Timer name copied: %s.\n", event.name);

    event.timeout_func = timeout_func;
    event.arg = arg;
    event.type = type;
    event.delay_time.tv_sec = period_us / 1000000; // 转换微秒到秒
    event.delay_time.tv_usec = period_us % 1000000;

    timeradd(&event.out_time, &event.delay_time, &event.out_time);
    printf("Step 5: Timer timeout values calculated.\n");

    stimer_queue *ptr = &stimer_queue_g;
    while (queue_checking)
    {
        usleep(1000); // 等待 1 毫秒
    }
    pthread_mutex_lock(&ptr->lock);
    printf("Step 6: Mutex locked.\n");

    /* 插入有序队列 */
    stimer_queue_order_insert(&event);
    printf("Step 7: Timer added to queue.\n");

    /* 检查队列并启动定时器 */
    if (ptr->start_flag == 0)
    {
        printf("Step 8: Starting timer queue for the first time.\n");
        ptr->start_flag = 1;
        stimer_queue_start();
        goto end;
    }

    if (timercmp(&(event.out_time), &(ptr->next_outtime), <))
    {
        printf("Step 9: New event is earlier, restarting timer queue.\n");
        stimer_queue_start();
        goto end;
    }

end:
    pthread_mutex_unlock(&ptr->lock);
    printf("Step 10: Mutex unlocked.\n");
    printf("Timer '%s' added successfully!\n", timer_name);
    return 0;
}

void stimer_queue_test_callback_period(void *arg)
{
    int *cnt = (int *)arg;
    LOG("period_cnt=%d", *cnt);
    //    LOG("");
}
void stimer_queue_test_callback_single(void *arg)
{
    int *cnt = (int *)arg;
    LOG("single_cnt=%d", *cnt);
    //    LOG("");
}
void stimer_queue_test_callback_single2(void *arg)
{
    char *str = (char *)arg;
    LOG("%s", str);
    //    LOG("");
}
void stimer_queue_test_callback_single3(void *arg)
{
    LOG("test-single3");
    //    LOG("");
}

void stimer_queue_test_thread(void)
{
    uint32_t cnt = 0, i;
    // 初始化随机数生成器
    srand((unsigned int)time(NULL));
#define rand_delay (rand() % 10 * 1000)

    // 生成一个周期定时器
    stimer_queue_add("period", stimer_queue_test_callback_period, &cnt, 1, 1000);
    // 生成多个单次随机延时的定时器
    for (i = 0; i < 5; i++)
    {
        stimer_queue_add("single1", stimer_queue_test_callback_single, &cnt, 0, rand_delay);
        stimer_queue_add("single2", stimer_queue_test_callback_single2, "single2", 0, rand_delay);
        stimer_queue_add("single3", stimer_queue_test_callback_single3, NULL, 0, rand_delay);
    }
    while (1)
    {
        usleep(rand_delay);
        // stimer_queue_add("test2", timer_queue_test, &cnt, 4, 0, 150);
        cnt++;
    }
}

/*int stimer_queue_test()
{
    LOG("create timer queue task.");

    pthread_t tid;
    int ret;
    //void *retval;
    ret = pthread_create(&tid, NULL, stimer_queue_test_thread, NULL);
    if(ret != 0) {
        LOG("create failed.");
        return -1;
    }
    return 0;
}*/

int32_t stimer_queue_init()
{
    /* 初始化定时器事件队列 */
    cvector_init(&stimer_queue_g.queue);

    /* 初始化队列锁 */
    pthread_mutex_init(&stimer_queue_g.lock, NULL);

    stimer_queue_g.start_flag = 0;
    LOG("init timer queue done.");

    // 自测程序
    // stimer_queue_test();
    return 0;
}
long getCurrentMicroseconds()
{
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    return currentTime.tv_usec;
}

// 获取当前时间（秒级）
int getCurrentSecond()
{
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);
    struct tm *localTime = localtime(&currentTime.tv_sec);

    int second = localTime->tm_sec; // 秒数

    return second;
}
