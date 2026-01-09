#include <stdint.h>

/*
 * 定时器事件封装
 * 定时器事件初始化和调用接口
 */
int32_t stimer_queue_init();

/* 以毫秒为单位的定时器队列添加函数 */
int stimer_queue_add(
    const char *timer_name,
    void (*timeout_func)(void *), // 超时处理函数指针
    void *arg,                    // 超时处理函数入参
    uint32_t type,                // 0表示一次性定时器，1表示周期定时器
    uint32_t period_ms            // 当前时刻延时时间，精度为毫秒
);

/* 以微秒为单位的定时器队列添加函数 */
int stimer_queue_add_us(
    const char *timer_name,
    void (*timeout_func)(void *), // 超时处理函数指针
    void *arg,                    // 超时处理函数入参
    uint32_t type,                // 0表示一次性定时器，1表示周期定时器
    uint32_t period_us            // 当前时刻延时时间，精度为微秒
);
long getCurrentMicroseconds();
int getCurrentSecond();
int stimer_queue_remove(const char *timer_name);