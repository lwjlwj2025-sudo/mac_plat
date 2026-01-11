#include <limits.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <stdbool.h>
#include <pthread.h>
#include <syslog.h>
// #include "mac_process.h"
#include "LEO_ra_config.h"
// #include "config.h"
// #include "ra_procedure.h"
#include "timer_queue.h"

#define MAX_HARQ_RETRANS 3 // 最大HARQ重传次数
UserInfo ueList[MAX_UE];
MacRarContext mac_rar_ctx = {0};
// 初始化UE列表
void initUeList()
{
    for (int i = 0; i < MAX_UE; i++)
    {
        ueList[i].id = i;
        ueList[i].isAccessGranted = 0;
    }
}

UserInfo *findUeById(int ueId)
{
    for (int i = 0; i < MAX_UE; i++)
    {
        if (ueList[i].id == ueId && ueList[i].isAccessGranted == 0)
        {
            return &ueList[i];
        }
    }
    return NULL;
}
static const u8 MAC_INFOR[17] = {0xA2, 0xD5, 0xA0, 0x8A, 0x0C, 0x58, 0xE1, 0x14, 0xDC,
                                 0x5E, 0x00, 0x64, 0x15, 0xC0, 0x25, 0x00, 0xFC}; // 用于测试
void msg2_send(UserInfo *ue)
{
    // UE *ue = findUeById(ueId);
    if (ue == NULL)
    {
        printf("UE %d not found\n", ue->id);
        return;
    }
    // u8 *dlsch_buffer=nr_fill_rar();
    memcpy(mac_rar_ctx.rar_buffer, MAC_INFOR, sizeof(MAC_INFOR) / sizeof(MAC_INFOR[0]));
    mac_rar_ctx.rar_buffer_size = sizeof(mac_rar_ctx.rar_buffer) / sizeof(mac_rar_ctx.rar_buffer[0]);
    ue->isAccessGranted = 1;
    // ue->harqRetransCount = 0;
    // ue->accessAttempt = 0;
    LOG("Sending RAR to UE %d\n", ue->id);
}

u8 *nr_fill_rar(NR_RA_t *ra_bs, u8 *dlsch_buffer, nr_pusch_pdu_t *pusch_pdu)
{
    NR_RA_HEADER_BI *rarbi = (NR_RA_HEADER_BI *)dlsch_buffer;
    NR_RA_HEADER_RAPID *rarh = (NR_RA_HEADER_RAPID *)(dlsch_buffer + 1);
    NR_MAC_RAR *rar = (NR_MAC_RAR *)(dlsch_buffer + 2);
    u8 csi_req = 0, tpc_command;

    tpc_command = 3; // this is 0 dB

    /// E/T/R/R/BI subheader ///
    // E = 1, MAC PDU includes another MAC sub-PDU (RAPID)
    // T = 0, Back-off indicator subheader
    // R = 2, Reserved
    // BI = 0, 5ms
    rarbi->E = 1;
    rarbi->T = 0;
    rarbi->R = 0;
    rarbi->BI = 0;

    /// E/T/RAPID subheader ///
    // E = 0, one only RAR, first and last
    // T = 1, RAPID
    rarh->E = 0;
    rarh->T = 1;
    rarh->RAPID = ra_bs->preamble_index;

    /// RAR MAC payload ///
    rar->R = 0;

    // TA command
    rar->TA1 = (u8)(ra_bs->timing_offset >> 5);   // 7 MSBs of timing advance
    rar->TA2 = (u8)(ra_bs->timing_offset & 0x1f); // 5 LSBs of timing advance

    // TC-RNTI
    rar->TCRNTI_1 = (u8)(ra_bs->rnti >> 8);   // 8 MSBs of rnti
    rar->TCRNTI_2 = (u8)(ra_bs->rnti & 0xff); // 8 LSBs of rnti

    // UL grant

    ra_bs->msg3_TPC = tpc_command;

    // if (pusch_pdu->frequency_hopping)
    //  AssertFatal(1==0,"PUSCH with frequency hopping currently not supported");

    int bwp_size = pusch_pdu->bwp_size;
    int prb_alloc = PRBalloc_to_locationandbandwidth0(ra_bs->msg3_nb_rb, ra_bs->msg3_first_rb, bwp_size);
    int valid_bits = 14;
    int f_alloc = prb_alloc & ((1 << valid_bits) - 1);

    u32 ul_grant = csi_req | (tpc_command << 1) | (pusch_pdu->mcs_index << 4) | (ra_bs->Msg3_tda_id << 8) | (f_alloc << 12) | (pusch_pdu->frequency_hopping << 26);

    rar->UL_GRANT_1 = (u8)(ul_grant >> 24) & 0x07;
    rar->UL_GRANT_2 = (u8)(ul_grant >> 16) & 0xff;
    rar->UL_GRANT_3 = (u8)(ul_grant >> 8) & 0xff;
    rar->UL_GRANT_4 = (u8)ul_grant & 0xff;

    int mcs = (u8)(rar->UL_GRANT_4 >> 4);
    // time alloc
    int Msg3_t_alloc = (u8)(rar->UL_GRANT_3 & 0x0f);
    // frequency alloc
    int Msg3_f_alloc = (u16)((rar->UL_GRANT_3 >> 4) | (rar->UL_GRANT_2 << 4) | ((rar->UL_GRANT_1 & 0x03) << 12));
    // frequency hopping
    int freq_hopping = (u8)(rar->UL_GRANT_1 >> 2);
    // TA command
    int ta_command = rar->TA2 + (rar->TA1 << 5);
    // TC-RNTI
    int t_crnti = rar->TCRNTI_2 + (rar->TCRNTI_1 << 8);
    return dlsch_buffer;
}
// 处理UE的随机接入请求
void *handleRandomAccessRequest(void *arg)
{

    initUeList();
    int ueId = 1;

    // MacRarContext *mac_rar_ctx = (MacRarContext *)malloc(sizeof(MacRarContext));
    // if (mac_rar_ctx == NULL)
    // {
    //     fprintf(stderr, "mac_pdu configuration is NULL\n");
    //     pthread_exit(NULL); // 退出线程
    // }
    UserInfo *ue = findUeById(ueId);
    if (ue == NULL)
    {
        printf("UE %d not found\n", ueId);
        pthread_exit(NULL); // 退出线程
    }

    // 检查UE是否已经获得随机接入许可
    if (ue->isAccessGranted)
    {
        printf("UE %d already has access\n", ueId);
        pthread_exit(NULL); // 退出线程
    }

    // UE发送随机接入前导码（PRACH）
    // printf("UE %d sending PRACH\n", ueId);
    pthread_mutex_lock(&lock);
    pthread_cond_wait(&cond2, &lock);
    // long time_1=getCurrentMicroseconds();

    LOG("Thread leo: Resuming execution after thread ue signaled.\n");
    // pthread_mutex_unlock(&lock);
    //  基站检测到PRACH并发送随机接入响应（RAR）
    if (detectPrach())
    {
        LOG("Base station detected PRACH from UE %d\n", ueId);
        msg2_send(ue);
    }
    else
    {
        printf("Base station failed to detect PRACH from UE %d\n", ueId);
        ue->accessAttempt++;
        if (ue->accessAttempt >= MAX_HARQ_RETRANS)
        {
            printf("UE %d access attempt limit reached\n", ueId);
        }
        else
        {
            printf("UE %d will retry access\n", ueId);
        }
    }
    long time_2 = getCurrentMicroseconds();
    LOG("Thread leo: Condition met, signaling thread ue to proceed. \n");
    // pthread_mutex_lock(&lock);
    pthread_cond_signal(&cond1);
    pthread_mutex_unlock(&lock);

    return NULL;
}
/* TS 38.214 ch. 6.1.2.2.2 - Resource allocation type 1 for DL and UL */
int PRBalloc_to_locationandbandwidth0(int NPRB, int RBstart, int BWPsize)
{
    // AssertFatal(NPRB>0 && (NPRB + RBstart <= BWPsize),
    //  "Illegal NPRB/RBstart Configuration (%d,%d) for BWPsize %d\n",
    //   NPRB, RBstart, BWPsize);

    if (NPRB <= 1 + (BWPsize >> 1))
        return (BWPsize * (NPRB - 1) + RBstart);
    else
        return (BWPsize * (BWPsize + 1 - NPRB) + (BWPsize - 1 - RBstart));
}
u16 detectPrach()
{
    LOG("Base station is listening on PRACH resources for incoming preambles...\n");
    for (int i = 0; i < MAX_PRACH_OCCASIONS; i++)
    {
        if (prachResources[i].isOccupied)
        {
            LOG("PRACH resource %d is occupied \n", prachResources[i].index);
            return i;
        }
    }
    return 0;
}