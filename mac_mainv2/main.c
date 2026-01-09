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
#include "info_process.h"
#include "layer_info_process.h"
#include "mac_process.h"
#include "App_udp.h"
#include "topology.h"
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int main()
{
    // /* 系统初始化 */
    Global_queue_init();
    MAC_U_init();
    UDP_all_init();
    // // pcieInit();
    stimer_queue_init();
    LOG("Sat_mac_u System initialization is complete");
    printCurrentTime();
    // //*********************         开数据接收线程          *****************************//
    // 创建来自Sat RRC的Package接收子线程

    pthread_t RecvPackage_from_sat_rrc_tid;
    pthread_create(&RecvPackage_from_sat_rrc_tid, NULL, RecvPackage_from_Sat_RRC, NULL);
    LOG("从RRC获取数据的线程建立成功");

    // 创建来自Sat RLC的Package接收子线程
    pthread_t RecvPackage_from_sat_rlc_tid;

    pthread_create(&RecvPackage_from_sat_rlc_tid, NULL, RecvPackage_from_Sat_RLC, &mutex);
    LOG("从RLC获取数据的线程建立成功");
    // // 准备需要下发到物理层的包, 周期为0.5ms ,就是每个子帧发一次
    prepare_next_subframe_send_data(); // mac-phy发送数据的部分
    // 用定时器调用函数
    //  创建来自UE MAC-U的Package接收子线程
    /*pthread_t RecvPackage_from_ue_mac_u_tid;
    pthread_create(&RecvPackage_from_ue_mac_u_tid, NULL, RecvPackage_from_UE_MAC_U, &mutex);
    LOG("从UE获取数据的线程建立成功");*/
    // // MAC_to_PHY_test();

    // // random access procedure
    // pthread_t ue_thread_prach, bs_thread_prach;

    slot_division();
    stimer_queue_add_us("mac_slot_division", slot_division, NULL, 1, slot_division_period); // 每隔多少周期，更新一下时隙
    usleep(1000);
    stimer_queue_remove("mac_slot_division");
    /*if (stimer_queue_init() != 0) {
        perror("Failed to initialize timer queue");
        return 1;
    }*/
    /* if(pthread_create(&ue_thread,NULL,randomaccessProcedure,NULL)!=0){
         perror("failed to create UE thread");
         return 1;
     }
     if(pthread_create(&bs_thread, NULL, handleRandomAccessRequest, NULL)){
         perror("failed to create BS thread");
         return 1;
     }*/

    // 等待UE线程结束
    /*
    pthread_create(&ue_thread_prach, NULL, randomaccessProcedure, NULL);
    pthread_create(&bs_thread_prach, NULL, handleRandomAccessRequest, NULL);
    pthread_join(ue_thread_prach, NULL);
    pthread_join(bs_thread_prach, NULL);*/

    // 等待基站线程结束

    // resources_apply();
    void *ptr = NULL;
    pthread_join(RecvPackage_from_sat_rrc_tid, &ptr); // 等待线程终止并获取其退出状态
    pthread_join(RecvPackage_from_sat_rlc_tid, &ptr);
    // pthread_join(RecvPackage_from_ue_mac_u_tid, &ptr);
    return 0;
}