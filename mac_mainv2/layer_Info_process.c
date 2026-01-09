#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <semaphore.h>
#include <sys/socket.h>
#include "ra_procedure.h"
// #include "ra_config.h"
// #include "config.h"
#include "scheduling.h"
#include "LEO_ra_config.h"
#include "resource_apply.h"
#include "layer_info_process.h"
#include "App_udp.h"
// 接收来自Sat RLC的数据包的线程处理代码
sem_t semaphore;
u8 recv_rlc_data[rlc_max] = {0};
u32 recv_rlc_data_len = 0;
void *RecvPackage_from_Sat_RLC(void *arg)
{

    pthread_mutex_t *lock = (pthread_mutex_t *)arg; //

    // 加锁
    pthread_mutex_lock(lock); //
    if (lock == NULL)
    {
        fprintf(stderr, "Error: lock is NULL\n");
        return NULL; // 遇到空指针直接返回，避免段错误
    }

    // 尝试加锁
    int ret = pthread_mutex_lock(lock);
    if (ret != 0)
    {
        fprintf(stderr, "Error: failed to lock mutex\n");
        return NULL;
    }
    u8 RecvPackage_from_RLC_buff[512];
    u32 recv_len = 0;
    u32 recv_rlc_package_len = 0;
    u8 recv_package_data[500] = {0};
    // u8 recv_rrc_data;
    // UDP_recv_init();
    while (1)
    {
        BufferClear(RecvPackage_from_RLC_buff, sizeof(RecvPackage_from_RLC_buff));             // 将一个字节数组中的所有数据清零
        recv_len = UDP_recv_rlc(RecvPackage_from_RLC_buff, sizeof(RecvPackage_from_RLC_buff)); // 通过udp协议接收数据
        if (recv_len == -1)
        {
            perror("recvfrRecvPackage_from_RLC_buffom() error");
            break;
        }
        if (recv_len > 0)
        {
            for (int index = 0; index < recv_len; index++)
            {
                printf("%02x ", RecvPackage_from_RLC_buff[index]);
            }
            printf("\n");

            recvPackageHandle(RecvPackage_from_RLC_buff, recv_len, &Queue_UDP_recv_sat_rlc, from_sat_rlc_msgHandle);

            // LOG("%d \n", recv_rlc_data_len);
            // for (int index = 0; index < recv_rlc_data_len; index++)
            // {
            //     printf("%02x ", recv_package_data[index]);
            // }
            // printf("\n");
        }

        // if (sizeof(recv_rlc_data) > recv_rlc_package_len)
        // {
        //     // for(int index=0; index < recv_rrc_data_len; index++){
        //     //   recv_rrc_data[index]=recv_package_data[index];
        //     // }
        //     memcpy(recv_rlc_data, recv_package_data, recv_rlc_data_len);
        // }
        /*for(int index=0; index < recv_rrc_data_len; index++)
                {
                    printf("%02x ", recv_rrc_data[index]);
                }
                printf("\n");*/
        if (!close_rlc())
            break;
    }
    sem_post(&semaphore);
    pthread_mutex_unlock(lock);
}

void *RecvPackage_from_UE_MAC_U(void *arg)
{
    pthread_mutex_t *lock = (pthread_mutex_t *)arg; //

    // 加锁
    pthread_mutex_lock(lock); //
    u8 RecvPackage_from_UE_MAC_U_buff[51200];
    u32 recv_len = 0;
    while (1)
    {
        BufferClear(RecvPackage_from_UE_MAC_U_buff, sizeof(RecvPackage_from_UE_MAC_U_buff));
        recv_len = UDP_recv_mac_u(RecvPackage_from_UE_MAC_U_buff, sizeof(RecvPackage_from_UE_MAC_U_buff));
        if (recv_len > 0)
        {
            recvphyPackageHandle(RecvPackage_from_UE_MAC_U_buff, recv_len, &Queue_UDP_recv_ue_mac_u);
        }
    }
    pthread_mutex_unlock(lock);
}
// 接收来自Sat RRC的数据包的线程处理代码
void *RecvPackage_from_Sat_RRC(void *arg)
{
    u8 RecvPackage_from_RRC_buff[512];
    u32 recv_len = 0;
    while (1)
    {
        BufferClear(RecvPackage_from_RRC_buff, sizeof(RecvPackage_from_RRC_buff));             // 将一个字节数组中的所有数据清零
        recv_len = UDP_recv_rrc(RecvPackage_from_RRC_buff, sizeof(RecvPackage_from_RRC_buff)); // 通过udp协议接收数据
        if (recv_len > 0)
        {
            recvPackageHandle(RecvPackage_from_RRC_buff, recv_len, &Queue_UDP_recv_sat_rrc, from_sat_rrc_msgHandle);
        }
    }
}