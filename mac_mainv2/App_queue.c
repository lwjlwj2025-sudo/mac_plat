#include <stdlib.h>
#include <string.h>
#include "App_queue.h"

//实现队列的基本操作、初始化、
void App_queue_init(App_queue *p_queue)
{
    p_queue->head = p_queue->arr;
    p_queue->tail = p_queue->arr;
    //memset(p_queue->arr, 0, sizeof(p_queue->arr));
}

int App_queue_forward(App_queue *p_queue)
{
    int result = (p_queue->head <= p_queue->tail) ? QUEUE_TRUE : QUEUE_FALSE;
    return result;
}

int App_queue_size(App_queue *p_queue)
{
    int size = (QUEUE_TRUE==App_queue_forward(p_queue)) ?
               (p_queue->tail - p_queue->head) :
               (sizeof(p_queue->arr) + (p_queue->tail - p_queue->head));
    //LOGI("Queue size=%d", size);
    return size;
}
int App_queue_remain_size(App_queue *p_queue)
{
    int size = App_queue_size(p_queue);
    //LOGI("Queue size=%d", size);
    return (sizeof(p_queue->arr) - size);
}

int App_queue_empty(App_queue *p_queue)
{
    if(p_queue->head == p_queue->tail)
    {
        return QUEUE_TRUE;
    }
    else
    {
        return QUEUE_FALSE;
    }
}

int App_queue_full(App_queue *p_queue)
{
    int size = App_queue_size(p_queue);
    if(size >= sizeof(p_queue->arr))
    {
        return QUEUE_TRUE;
    }
    else
    {
        return QUEUE_FALSE;
    }
}

int App_queue_push(App_queue *p_queue, char *data, int len)   //入队
{
    int remain_size = App_queue_remain_size(p_queue);
    int remain_head_size, remain_tail_size;
    //LOGI("len=%d, remain_size=%d", len, remain_size);
    if(len < remain_size)
    {
        remain_head_size = (QUEUE_TRUE == App_queue_forward(p_queue)) ?
                           (p_queue->arr_tail - p_queue->head) :
                           (p_queue->tail - p_queue->head);
        if(remain_head_size > len)
        {
            memcpy(p_queue->head, data, len);
            p_queue->head += len;
        }
        else
        {
            remain_tail_size = len - remain_head_size;
            memcpy(p_queue->head, data, remain_head_size);
            memcpy(p_queue->arr, &data[remain_head_size], remain_tail_size);
            p_queue->head = p_queue->arr + remain_tail_size;
        }
    }
    else
    {
        return QUEUE_ERROR;
    }
    //LOGI("queue_size=%d", App_queue_size(p_queue));
    return QUEUE_OK;
}

int App_queue_pop(App_queue *p_queue, char *data, int MaxLen, int mask) //出队
{ 
    int size = App_queue_size(p_queue);
    int head_size, tail_size;
    int RealPopLen;

    RealPopLen = (MaxLen < size) ? MaxLen : size;
    //LOGI("RealPopLen=%d, size=%d, MaxLen=%d", RealPopLen, size, MaxLen);
    head_size = (QUEUE_TRUE == App_queue_forward(p_queue)) ?
                (p_queue->head - p_queue->tail) :
                (p_queue->arr_tail - p_queue->tail);
    if(head_size >= RealPopLen)
    {
        if(mask&0x01)
        {
            memcpy(data, p_queue->tail, RealPopLen);
        }
        if(mask&0x02)
        {
            p_queue->tail += RealPopLen;
        }
    }
    else
    {
        tail_size = RealPopLen - head_size;
        if(mask&0x01)
        {
            memcpy(data, p_queue->tail, head_size);
            memcpy(&data[head_size], p_queue->arr, tail_size);
        }
        if(mask&0x02)
        {
            p_queue->tail = p_queue->arr + tail_size;
        }
    }

    return RealPopLen;
}

int App_QueueWritePack(App_Queue_Stroe_s_type *Ptr, char *data, int len)
{
    //Debug_HEX_Print(data, len, "Find pack and write");
    if(Store_Queue_Num <= Ptr->capacity)
    {
        return QUEUE_FALSE;
        Ptr->TailNum = ((Store_Queue_Num-1) <= Ptr->TailNum) ? 0 : (Ptr->TailNum+1);
    }
    if(QUEUE_OK != App_queue_push(&(Ptr->Queue[Ptr->HeadNum]), data, len))
    {
        return QUEUE_FALSE;
    }
    Ptr->HeadNum = ((Store_Queue_Num-1) <= (Ptr->HeadNum)) ?
                    0 :
                    (Ptr->HeadNum + 1);
    Ptr->capacity = (Ptr->capacity+1);
    return QUEUE_TRUE;
}