#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include <math.h>
#include <unistd.h>

#include "config.h"
#include "mac_process.h"
#include "resource_apply.h"
#include "globals.h"
#include "CONSTANTS.h"
#include "timer_queue.h"
#define S_SIZE 14
#define L_SIZE 14

const int time_domain_table[S_SIZE][L_SIZE] = {
    {0, 14, 28, 42, 56, 70, 84, 98, 97, 83, 69, 55, 41, 27},
    {1, 15, 29, 43, 57, 71, 85, 99, 96, 82, 68, 54, 40},
    {2, 16, 30, 44, 58, 72, 86, 100, 95, 81, 67, 53},
    {3, 17, 31, 45, 59, 73, 87, 101, 94, 80, 66},
    {4, 18, 32, 46, 60, 74, 88, 102, 93, 79},
    {5, 19, 33, 47, 61, 75, 89, 103, 92},
    {6, 20, 34, 48, 62, 76, 90, 104},
    {7, 21, 35, 49, 63, 77, 91},
    {8, 22, 36, 50, 64, 78},
    {9, 23, 37, 51, 65},
    {10, 24, 38, 52},
    {11, 25, 39},
    {12, 26},
    {13}};
u8 BSR_TABLE(u8 BS_value)
{
    int index = 0;
    if (BS_value == 0)
        index = 0;
    else if (BS_value >= 1 && BS_value <= 10)
        index = 1;
    else if (BS_value >= 11 && BS_value <= 14)
        index = 2;
    else if (BS_value >= 15 && BS_value <= 20)
        index = 3;
    else if (BS_value >= 21 && BS_value <= 28)
        index = 4;
    else if (BS_value >= 29 && BS_value <= 38)
        index = 5;
    else if (BS_value >= 39 && BS_value <= 53)
        index = 6;
    else if (BS_value >= 54 && BS_value <= 74)
        index = 7;
    else if (BS_value >= 75 && BS_value <= 102)
        index = 8;
    else if (BS_value >= 103 && BS_value <= 142)
        index = 9;
    else if (BS_value >= 143 && BS_value <= 198)
        index = 10;
    else if (BS_value >= 199 && BS_value <= 276)
        index = 11;
    else if (BS_value >= 277 && BS_value <= 384)
        index = 12;
    else if (BS_value >= 385 && BS_value <= 535)
        index = 13;
    else if (BS_value >= 536 && BS_value <= 745)
        index = 14;
    else if (BS_value >= 746 && BS_value <= 1038)
        index = 15;
    else if (BS_value >= 1039 && BS_value <= 1446)
        index = 16;
    else if (BS_value >= 1447 && BS_value <= 2014)
        index = 17;
    else if (BS_value >= 2015 && BS_value <= 2806)
        index = 18;
    else if (BS_value >= 2807 && BS_value <= 3909)
        index = 19;
    else if (BS_value >= 3910 && BS_value <= 5446)
        index = 20;
    else if (BS_value >= 5447 && BS_value <= 7587)
        index = 21;
    else if (BS_value >= 7588 && BS_value <= 10570)
        index = 22;
    else if (BS_value >= 10571 && BS_value <= 14726)
        index = 23;
    else if (BS_value >= 14727 && BS_value <= 20516)
        index = 24;
    else if (BS_value >= 20517 && BS_value <= 28581)
        index = 25;
    else if (BS_value >= 28582 && BS_value <= 39818)
        index = 26;
    else if (BS_value >= 39819 && BS_value <= 55474)
        index = 27;
    else if (BS_value >= 55475 && BS_value <= 77284)
        index = 28;
    else if (BS_value >= 77285 && BS_value <= 107669)
        index = 29;
    else if (BS_value >= 107670 && BS_value <= 150000)
        index = 30;
    else
        index = 31;
    return index;
}

// App_queue *waittingdata_queue[MAX_Logical_num];
ue_schedule *mac_ue_info = NULL;

static ue_resource_apply ue_re_apply;
u8 Logicalchannel_G_match[3] = {1, 2, 2};   // 逻辑信道和逻辑信道组的对应关系，设置三个逻辑信道
void init_logical(ue_schedule *mac_ue_info) // 初始化逻辑信道
{
    mac_ue_info->numChannels = sizeof(mac_ue_info->logicalchannel) / sizeof(mac_ue_info->logicalchannel[0]);
    mac_ue_info->TB = 0;

    mac_ue_info->uplinkGrant.ULGrant = 0;
    mac_ue_info->logicalchannel[0].LCID = 1;
    mac_ue_info->logicalchannel[1].LCID = 2;
    mac_ue_info->logicalchannel[2].LCID = 3;
    mac_ue_info->logicalchannel[0].priority = 1;
    mac_ue_info->logicalchannel[1].priority = 2;
    mac_ue_info->logicalchannel[2].priority = 2;
    for (int i = 0; i < mac_ue_info->numChannels; i++)
    {
        init_Logical(&mac_ue_info->logicalchannel[i]);
    }
}
u8 mac_status_logicalgroup_data(u8 LCG_data[])
{ // 整合一个逻辑信道组的数据量

    RLC_MAC_data_test(); // 测试数据输入
    // mac_ue_info = (ue_schedule *)buffer;
    u16 i, j, LCID, LCGID;
    for (i = 0; i < mac_ue_info->numChannels; i++)
    {
        LCID = mac_ue_info->logicalchannel[i].LCID;
        LCGID = Logicalchannel_G_match[LCID - 1];
        for (j = 0; j < mac_ue_info->logicalchannel[i].PDUnum; j++)
        {
            // waittingdatabuffer_queue(LCID, mac_ue_info->logicalchannel[i].PDUsize[j]); // 统计每个逻辑信道的待传数据量

            mac_ue_info->logicalchannel[i].waitingdatabuffer += mac_ue_info->logicalchannel[i].PDUsize[j]; // 统计逻辑信道中等待发送的数据量
            LCG_data[LCGID - 1] += mac_ue_info->logicalchannel[i].PDUsize[j];                              // 统计整个逻辑信道数据组的数据量
        }
        if (mac_ue_info->logicalchannel[i].waitingdatabuffer > 0)
        {
            mac_ue_info->logicalchannel[i].logicalchannelSR_mask = true;
        }
    }
    mac_ue_info->BSR[0] = LCG_data[0];
    mac_ue_info->BSR[1] = LCG_data[1];
    int k = 0;
    sortLevel_1(mac_ue_info->logicalchannel, mac_ue_info->numChannels); // 给逻辑信道优先级从高到低排序
    for (k = 1; k <= mac_ue_info->numChannels; k++)
    {
        if (mac_ue_info->logicalchannel->logicalchannelSR_mask == true)
        {
            ue_re_apply.ue_state = sr_snd; // 状态切换到发送SR
            break;
        }
    }
    // if(sizeof(waittingdata_queue)/sizeof(waittingdata_queue[0])>0){ //有数据在逻辑信道中等待发送  ?wrong
    //     apply_state=sr_snd;
    // }
    LOG("DATA get end, in  logical %d send SR\n", k);
    return k; // 返回是哪个逻辑信道触发了SR
}
void DCI_get_from_LEO() {}
DCI_TB_info *handle_DCI(u8 *buffer)
{ // 处理DCI

    DCI_0_0 *recv_dci = (DCI_0_0 *)buffer;
    DCI_TB_info *ue_tb_alloc = (DCI_TB_info *)malloc(sizeof(DCI_TB_info));
    u16 RIV;
    ue_re_apply.is_grant_received = true;
    RIV = recv_dci->RIV_2 + (recv_dci->RIV_1 << 8);
    u8 MCS = recv_dci->MCS_2 + (recv_dci->MCS_1 << 2);
    ue_tb_alloc->per_RB_bytenum = cal_RB_carry_capacity(MCS);
    u8 time_domain = recv_dci->time_domain;
    u8 RB_total = RB_TOTALNUM;               // 整个系统带宽内可用的资源块数量
    ue_tb_alloc->RBG = (RIV / RB_total) + 1; // 资源块组的大小，该组包含的连续资源块的数量 RBG-1 应小于等于RBtotal的一半，这个公式才有效
    ue_tb_alloc->RB = RIV % RB_total;        // 在当前带宽内的资源块索引
    bool found = false;                      // 标志变量，记录是否找到目标值
    for (int i = 0; i < S_SIZE && !found; i++)
    {
        for (int j = 0; j < L_SIZE && !found; j++)
        {
            if (time_domain_table[i][j] == time_domain)
            {
                ue_tb_alloc->L = j + 1; // 时域上持续的符号长度
                ue_tb_alloc->S = i;     // 开始的符号位置
                found = true;           // 设置标志为 true
            }
        }
    }
    if (found)
    {
        printf("Found at Start = %d, Length = %d\n", ue_tb_alloc->S, ue_tb_alloc->L);
    }
    else
    {
        printf("Value not found.\n");
    }
    // ue_tb_alloc->L = (time_domain / 14) + 1; // 时域上持续的符号长度
    // ue_tb_alloc->S = time_domain % 14;       // 开始的符号位置
    return ue_tb_alloc;
}
u8 *trigger_BSR(u8 short_bsrtrigger_flag, u8 LCG_data[])
{
    // 以LCG为单位进行BSR触发
    // bsr_trigger=1
    mac_ce_BSR *mac_ce_bsr = (mac_ce_BSR *)malloc(sizeof(mac_ce_BSR));

    u8 BSRIndex = 0;
    if (short_bsrtrigger_flag == 1)
    {
        mac_ce_bsr->R = 0;
        mac_ce_bsr->LCG_ID = 1;                                                         // 假设传输ue的所有数据
        mac_ce_bsr->LCID = 61;                                                          // 6.2.1-2 in 38.321表格中设置的，短BSR
        mac_ce_bsr->Buffer_Size = BSR_TABLE(mac_ue_info->BSR[0] + mac_ue_info->BSR[1]); // BSR容量与index的对应关系表
        // for (int i = 0; i < 2; i++) // 两个逻辑信道组
        // {
        //     if (mac_ue_info->BSR[i] > 0)
        //     { // 组装包含bsr的mac ce

        //         // App_queue_push(mac_ce_bsr,mac_ce,sizeof(mac_ce));
        //         mac_ce_bsr->Buffer_Size = BSR_TABLE(mac_ue_info->BSR[i]); // BSR容量与index的对应关系表
        //         u8 *mac_ce = (u8 *)mac_ce_bsr;
        //         App_queue_init(&mac_ce_pdu[i]);
        //         App_queue_push(&mac_ce_pdu[i], mac_ce, sizeof(mac_ce_bsr)); // 在缓冲区存储每个逻辑信道组的mac ce数据
        //     }
        // }
    }
    u8 *mac_ce = (u8 *)mac_ce_bsr;
    short_bsrtrigger_flag = 0;
    return mac_ce;
}
u8 send_BSR(DCI_TB_info *ul_grant, u8 LCG_data[], u8 *bsr_to_phy)
{
    printf("UE in bsr_snd state\n");
    u8 BSRIndex;

    // 发送 BSR 报告
    u8 BSR_trigger = 1;

    printf("Sending BSR report...\n");
    bsr_to_phy = trigger_BSR(BSR_trigger, LCG_data); // 生成组装的mac ce
    // for (int i = 0; i < sizeof(mac_ce_BSR); i++)
    // {
    //     printf("%d\n", bsr_to_phy[i]);
    // }
    if (ul_grant->per_RB_bytenum * 8 * ul_grant->RBG >= sizeof(bsr_to_phy))
    {
        ue_re_apply.is_bsr_sent = true;
        LOG("send BSR success\n");
    }

    return 0;
}
u16 trigger_SR(u16 slot_per_frame_num, u8 SR_offset, u8 SR_period, u8 SR_send_num)
{
    u16 sfn_sf;
    u32 temp_result;
    printf("UE in sr_snd state\n");

    sfn_sf = frame_t->frame_num * slot_per_frame_num + frame_t->slot_num;

    printf("Scheduling Request active in frame %d slot %d \n", frame_t->frame_num, frame_t->slot_num);
    if ((sfn_sf - SR_offset) % SR_period == 0)
    { // SR_OFFSET在周期内的偏移量，以时隙为单位，sr_period几个时隙
        if (SR_send_num < ue_re_apply.sr_trans_max)
        {
            ue_re_apply.is_sr_sent = true;
            // MAC_TO_PHY_SEND_SR(); // interface 发送后结束，只发一次SR

            SR_send_num += 1;
            // 停止定时器
            LOG("Condition met, stopping 'mac_slot_division' timer.");
            stimer_queue_remove("mac_slot_division"); // 停止指定定时器
            // return 0;
        }
        else
        {
            LOG("SR发送达到上限,通过随机接入msg3来进行资源申请\n");
            // return 0;
        }

        // 没有配置禁止定时器
    }

    return 0;
    // get frame_num and slot_num
}
// void waittingdatabuffer_queue(u8 lcid, u8 pdusize)
// {
//     u8 *pdusize_0 = &pdusize;
//     App_queue_push(waittingdata_queue[lcid], &pdusize, sizeof(u8));
// }
/*void data_snd()
{
    printf("UE in data_snd state\n");

    if (ue_re_apply.is_grant_received && ue_re_apply.is_bsr_sent)
    {
        // 发送数据
        printf("Sending data...\n");

        // 如果新的 BSR 生成，复用到 PDU 中进行发送
        if (new_bsr_generated())
            send_new_bsr_in_pdu();

        // 判断是否需要重新发送 SR 请求
        if (time_to_send_sr())
        {
            printf("Time for new SR request, moving back to sr_snd state.\n");
            ue_re_apply.ue_state = sr_snd;
        }
    }
}*/
void resources_apply()
{ //
    // ue_schedule *mac_ue_info=(ue_schedule*)malloc(sizeof(ue_schedule));
    // App_queue *waittingdata_queue=(App_queue*)malloc(MAX_Logical_num * sizeof(App_queue));
    // ue_re_apply.bsr_trans_max = BSR_TRANS_MAX;
    if (mac_ue_info == NULL)
    {
        mac_ue_info = malloc(sizeof(ue_schedule));
        if (mac_ue_info == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for mac_ue_info.\n");
            return;
        }
    }
    init_logical(mac_ue_info);
    ue_re_apply.has_data = false;
    ue_re_apply.is_bsr_sent = false;
    ue_re_apply.is_grant_received = false;
    ue_re_apply.is_TB_received = false;
    ue_re_apply.is_sr_sent = false;
    ue_re_apply.sr_periodicity = SR_PERIOD;
    ue_re_apply.sr_offset = SR_OFFSET;
    DSR_TRANSMAX_t dsr_TransMax = sr_n64; // 设置DSR（调度请求）传输最大次数为sr_n64
    ue_re_apply.sr_trans_max = 1 << (2 + dsr_TransMax);
    ue_re_apply.ue_state = schedule_idle;
    u8 LCG_DATA[LCG_NUM] = {0}; // 设置了两个逻辑信道组
    u8 LC_choose = 0;           // 选择发送哪个逻辑信道的数据
    u8 BSRIndex = 0;
    u8 SR_num_init = 0;
    u8 *BSR_to_PHY = NULL;

    DCI_TB_info *ul_grant = (DCI_TB_info *)malloc(sizeof(DCI_TB_info)); // 发送SR后传输的DCI中解出的资源分配，用以传输BSR
    DCI_TB_info *TB_t = (DCI_TB_info *)malloc(sizeof(DCI_TB_info));     // 发送BSR后传输的DCI中解出的资源分配，用以传输缓冲数据
    if (TB_t == NULL)
    {
        fprintf(stderr, "TB_t configuration is NULL\n");
        pthread_exit(NULL); // 退出线程
    }
    if (ul_grant == NULL)
    {
        fprintf(stderr, "ul_grant configuration is NULL\n");
        pthread_exit(NULL); // 退出线程
    }
    // frame_info *frame_t = (frame_info *)malloc(sizeof(frame_info));
    while (mac_ue_info->TB == 0) // 没有分配到资源则一直循环
    {
        switch (ue_re_apply.ue_state)
        {
        case schedule_idle:
            LC_choose = mac_status_logicalgroup_data(LCG_DATA); // 统计逻辑信道的数据量，并统计逻辑信道组的数据量
            break;
        case sr_snd:

            if (!ue_re_apply.is_grant_received && !ue_re_apply.is_sr_sent)
            {

                //
                trigger_SR(slot_per_frame, ue_re_apply.sr_offset, ue_re_apply.sr_periodicity, SR_num_init);
                // 没有配置禁止定时器
            }
            else if (ue_re_apply.is_sr_sent && !ue_re_apply.is_grant_received)
            {
                u8 *DCI_mes = GET_PDCCH_DCI_SR();
                if (DCI_mes != NULL)
                {
                    // 打印内存内容
                    for (int i = 0; i < 5; ++i)
                    {
                        printf("%02X ", DCI_mes[i]);
                    }
                    printf("\n");
                    ul_grant = handle_DCI(DCI_mes);
                    // 释放内存
                    free(DCI_mes);
                }

                // if (ul_grant->RBG > 0)
                // {
                //     ue_re_apply.is_grant_received = true;
                // }
            }
            else if (ue_re_apply.is_grant_received)
            {
                // 收到了上行授权，进入 BSR 发送状态
                printf("Received uplink grant, moving to BSR snd state.\n");
                ue_re_apply.ue_state = bsr_snd;
            }
            break;
        case bsr_snd:
            if (!ue_re_apply.is_TB_received && !ue_re_apply.is_bsr_sent)
            {
                send_BSR(ul_grant, LCG_DATA, BSR_to_PHY);
            }
            else if (ue_re_apply.is_bsr_sent && !ue_re_apply.is_TB_received)
            {
                u8 *DCI_message = GET_PDCCH_DCI_BSR();
                if (DCI_message != NULL)
                {
                    // 打印内存内容
                    for (int i = 0; i < 5; ++i)
                    {
                        printf("%02X ", DCI_message[i]);
                    }
                    printf("\n");
                    TB_t = handle_DCI(DCI_message);
                    ue_re_apply.is_TB_received = true;
                    // 释放内存
                    free(DCI_message);
                }
            }
            else if (ue_re_apply.is_TB_received)
            {
                // 收到了上行授权，进入数据发送状态
                mac_ue_info->TB = TB_t->RBG;

                printf("Received uplink grant, moving to data_snd state.\n");
                ue_re_apply.ue_state = data_snd;
            }

            break;
        case data_snd:
            // scheduling
            break;
        default:
            LOG("resource apply by SR is error!\n");
        }
    }
    // free(frame_t);
    free(ul_grant);
    free(TB_t);

    // u8 time1 = getCurrentMicroseconds();
    // if (getCurrentMicroseconds() < time1)
    // {
    // } // two slots window
}
void MAC_TO_PHY_SEND_BSR()
{
}
u8 *GET_PDCCH_DCI_SR()
{
    // 分配内存
    u8 *DCI_buf = malloc(5 * sizeof(u8));
    if (DCI_buf != NULL)
    { // 0000 0000 1111 1010 1110 0000 0100 0000 0000 0000
        // RIV=250 time_domain=14 MCS=1
        //  初始化内存
        DCI_buf[0] = 0x00;
        DCI_buf[1] = 0xFA; // RIV=250,RB=0,RBG=1,分配小内存传输BSR
        DCI_buf[2] = 0xE0;
        DCI_buf[3] = 0x40;
        DCI_buf[4] = 0x00;
    }
    else
    {
        // 处理内存分配失败的情况
        fprintf(stderr, "Memory allocation failed.\n");
        // 返回 NULL 表示失败
        return NULL;
    }

    // 返回分配好的内存
    return DCI_buf;
}
u8 *GET_PDCCH_DCI_BSR()
{
    // 分配内存
    u8 *DCI_buf = malloc(5 * sizeof(u8));
    if (DCI_buf != NULL)
    { // 0000 0000 1111 1010 1110 0000 0100 0000 0000 0000
        // RIV=250 time_domain=14 MCS=1
        //  初始化内存
        DCI_buf[0] = 0x00;
        DCI_buf[1] = 0xFA; // RIV=250,RB=0,RBG=1,分配小内存传输BSR
        DCI_buf[2] = 0xE0;
        DCI_buf[3] = 0x40;
        DCI_buf[4] = 0x00;
    }
    else
    {
        // 处理内存分配失败的情况
        fprintf(stderr, "Memory allocation failed.\n");
        // 返回 NULL 表示失败
        return NULL;
    }

    // 返回分配好的内存
    return DCI_buf;
}
void RLC_MAC_data_test() // rlc层传输下来的每个逻辑信道的数据
{

    mac_ue_info->logicalchannel[0].PDUnum = 4;
    mac_ue_info->logicalchannel[1].PDUnum = 3;
    mac_ue_info->logicalchannel[2].PDUnum = 3;
    // 初始化 PDU 大小
    mac_ue_info->logicalchannel[0].PDUsize[0] = 5;
    mac_ue_info->logicalchannel[0].PDUsize[1] = 4;
    mac_ue_info->logicalchannel[0].PDUsize[2] = 8;
    mac_ue_info->logicalchannel[0].PDUsize[3] = 6;

    mac_ue_info->logicalchannel[1].PDUsize[0] = 14;
    mac_ue_info->logicalchannel[1].PDUsize[1] = 3;
    mac_ue_info->logicalchannel[1].PDUsize[2] = 20;

    mac_ue_info->logicalchannel[2].PDUsize[0] = 15;
    mac_ue_info->logicalchannel[2].PDUsize[1] = 9;
    mac_ue_info->logicalchannel[2].PDUsize[2] = 1;
}
