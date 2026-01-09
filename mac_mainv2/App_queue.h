#ifndef _APP_QUEUE_H_
#define _APP_QUEUE_H_

#define QUEUE_OK            0
#define QUEUE_ERROR         -1

#define QUEUE_TRUE          1
#define QUEUE_FALSE         0

#define QUEMAXSIZE          2000
#define Store_Queue_Num     10

typedef struct
{
    char arr[QUEMAXSIZE];
    char arr_tail[0];
    char *head;
    char *tail;
}App_queue;

typedef struct
{
    int capacity;
    int HeadNum;
    int TailNum;
    App_queue Queue[Store_Queue_Num];
}App_Queue_Stroe_s_type;


void App_queue_init(App_queue *p_queue);
int App_queue_forward(App_queue *p_queue);
int App_queue_size(App_queue *p_queue);
int App_queue_remain_size(App_queue *p_queue);
int App_queue_empty(App_queue *p_queue);
int App_queue_full(App_queue *p_queue);
int App_queue_push(App_queue *p_queue, char *data, int len);
int App_queue_pop(App_queue *p_queue, char *data, int MaxLen, int mask);

int App_QueueWritePack(App_Queue_Stroe_s_type *Ptr, char *data, int len);
int App_QueueReadPack(App_Queue_Stroe_s_type *Ptr, char *data, int MaxLen);


#endif
