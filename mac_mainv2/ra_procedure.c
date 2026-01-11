// ue random access cfra two-steps
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
#include "mac_process.h"
// #include "ra_config.h"
// #include "config.h"
// #include "ra_procedure.h"
#include "timer_queue.h"
// #include "ra_procedure.h"

#define NCS 2 // NCS config is 0,PRACH format4  xunhuanyiweichangdu
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
UE_MAC_INST_t *get_mac_inst(module_id_t module_id)
{
    return &nr_ue_mac_inst[(int)module_id];
}
PRACHResource prachResources[MAX_PRACH_OCCASIONS];
const u16 table_7_2_1[16] = {
    // 对应的BI值
    5,    // row index 0
    10,   // row index 1
    20,   // row index 2
    30,   // row index 3
    40,   // row index 4
    60,   // row index 5
    80,   // row index 6
    120,  // row index 7
    160,  // row index 8
    240,  // row index 9
    320,  // row index 10
    480,  // row index 11
    960,  // row index 12
    1920, // row index 13
};
// u16 send_preamble_flag=0;
u16 rar_received_flag = 0;
void init_RA(RA_config_t *ra, PRACH_RESOURCES_t *prach_resources)
{
    // UE_MAC_INST_t *mac=get_mac_inst(mod_id);
    // mac->state=UE_PERFORMING_RA;
    // RA_config_t *ra=&mac->ra;
    // PRACH_RESOURCES_t *prach_resources=&ra->prach_resources;
    ra->RA_active = 1;
    ra->ra_PreambleIndex = -1;
    ra->RA_usedGroupA = 1;
    ra->RA_RAPID_found = 0;
    ra->preambleTransMax = 0;
    // ra->first_Msg3           = 1;
    ra->starting_preamble_nb = 0;
    ra->RA_backoff_cnt = 0;
    ra->RA_window_cnt = -1;
    // ra->cfra=1;  //cfra
    prach_resources->RA_PREAMBLE_BACKOFF = 0;

    prach_resources->RA_PREAMBLE_TRANSMISSION_COUNTER = 1;
    prach_resources->RA_PREAMBLE_POWER_RAMPING_COUNTER = 1;

    prach_resources->RA_SCALING_FACTOR_BI = 1;
    // u16 n_prbs=get_N_RA_RB(0,0);
    // int start_prb=rrc_config->msg1_FrequencyStart+rrc_config->BWPStart;
}
static const u16 N_RA_RB[16] = {6, 3, 2, 24, 12, 6, 12, 6, 3, 24, 12, 6, 12, 6, 24, 12};
/*u16 get_N_RA_RB(int delta_f_RA_PRACH,int delta_f_PUSCH) {   //get rach need RB 38211 6.3.3.2-1


    u8 index = 0;
    switch(delta_f_RA_PRACH) {
        case 0 :
        index = 6;
        if (delta_f_PUSCH == 0)
            index += 0;
        else if(delta_f_PUSCH == 1)
            index += 1;
        else
            index += 2;
        break;
        case 1 :
        index = 9;
        if (delta_f_PUSCH == 0)
            index += 0;
        else if(delta_f_PUSCH == 1)
            index += 1;
        else
            index += 2;
        break;
        case 2 :
        index = 11;
        if (delta_f_PUSCH == 2)
            index += 0;
        else
            index += 1;
        break;
        case 3:
        index = 13;
        if (delta_f_PUSCH == 2)
            index += 0;
        else
            index += 1;
        break;
    default : index = 10;/*30khz prach scs and 30khz pusch scs
  }
  return N_RA_RB[index];
}	*/

u16 get_DELTA_PREAMBLE(u16 prach_format)
{
    msg1_SubcarrierSpacing scs;
    scs = 3; // 120khz
    // msg1_SubcarrierSpacing scs=mac->ra.scs;
    u8 prachConfigIndex, mu;
    int prach_preamble_length = PREAMBLE_LENGTH;
    switch (scs)
    {
    case NR_SubcarrierSpacing_kHz15:
        mu = 0;
        break;

    case NR_SubcarrierSpacing_kHz30:
        mu = 1;
        break;

    case NR_SubcarrierSpacing_kHz60:
        mu = 2;
        break;

    case NR_SubcarrierSpacing_kHz120:
        mu = 3;
        break;

    case NR_SubcarrierSpacing_kHz240:
        mu = 4;
        break;

    case NR_SubcarrierSpacing_kHz480:
        mu = 5;
        break;

    case NR_SubcarrierSpacing_kHz960:
        mu = 6;
        break;

    default:
        printf("Unknown msg1_SubcarrierSpacing ");
    }
    if (prach_preamble_length == 139)
    {

        switch (prach_format)
        {
            // short preamble formats
        case 0:
        case 3:
            return 8 + 3 * mu;

        case 1:
        case 4:
        case 8:
            return 5 + 3 * mu;

        case 2:
        case 5:
            return 3 + 3 * mu;

        case 6:
            return 3 * mu;

        case 7:
            return 11 + 3 * mu;

        default:
            break;
            // printf("[UE %d] ue_procedures.c: FATAL, Illegal preambleFormat %d, prachConfigIndex %d\n", mod_id, prach_format, prachConfigIndex);
        }
    }
    else
    {

        switch (prach_format)
        {

        // long preamble formats
        case 0:
        case 3:
            return 0;

        case 1:
            return -3;

        case 2:
            return -6;
        }
    }
}
// TS 38.321 subclause 5.1.3 - RA preamble transmission - ra_PREAMBLE_RECEIVED_TARGET_POWER configuration
// Measurement units:
// - preambleReceivedTargetPower      dBm (-202..-60, 2 dBm granularity)
// - delta_preamble                   dB
// - RA_PREAMBLE_POWER_RAMPING_STEP   dB
// - POWER_OFFSET_2STEP_RA            dB
// returns receivedTargerPower in dBm
u16 get_power_prach(RA_config_t *ra)
{
    u8 delta_preamble;
    u8 receivedTargerPower;
    ra->prach_resources.prach_format = 3;
    delta_preamble = get_DELTA_PREAMBLE(ra->prach_resources.prach_format);
    receivedTargerPower = ra->rach_configdedicated.preambleReceivedTargetPower + delta_preamble + (ra->prach_resources.RA_PREAMBLE_POWER_RAMPING_COUNTER - 1) * ra->prach_resources.RA_PREAMBLE_POWER_RAMPING_STEP;
    // LOG( "In %s: receivedTargerPower is %d dBm \n", __FUNCTION__, receivedTargerPower);

    return receivedTargerPower;
}
u16 get_ra_rnti(RA_config_t *ra)
{

    u8 s_id = ra->prach_resources.s_id;
    u8 t_id = ra->prach_resources.t_id;
    u8 f_id = ra->prach_resources.f_id;
    u8 ul_carrier_id = 0; // NUL
    u16 ra_rnti = 1 + s_id + 14 * t_id + 14 * 80 * f_id + 14 * 80 * 8 * ul_carrier_id;
    // LOG("Computed ra_RNTI is %x \n", ra->ra_rnti);
    return ra_rnti;
}
void get_prach_resources(module_id_t mod_id)
{
    UE_MAC_INST_t *mac = get_mac_inst(mod_id);
    RA_config_t *ra = &mac->ra;
    PRACH_RESOURCES_t *prach_resources = &ra->prach_resources;
    if (ra->rach_configdedicated.cfra)
    {
        ra->ra_PreambleIndex = 0; // write a ssb resouce list
    }
    if (prach_resources->RA_PREAMBLE_TRANSMISSION_COUNTER > 1)
        prach_resources->RA_PREAMBLE_POWER_RAMPING_COUNTER++;
    // prach_resources->ra_PREAMBLE_RECEIVED_TARGET_POWER = get_power_prach(mod_id,prach_resources,ra);
}

u16 get_max_SSB_RSRP(RACH_Config_rrc *rrc_config)
{ // 选择具有最大参考信号接受功率的SSB
    u16 MAX_SSB_INDEX = 0;
    for (int i = 1; i < SSB_num; i++)
    {
        if (SSB_RSRP[i] > SSB_RSRP[MAX_SSB_INDEX])
        {
            MAX_SSB_INDEX = i;
        }
    }
    if (SSB_RSRP[MAX_SSB_INDEX] < rrc_config->rsrp_ThresholdSSB)
    {
        MAX_SSB_INDEX = rand() % SSB_num;
    }
    return MAX_SSB_INDEX;
}
u8 msg1_transmitted(RA_config_t *ra, u16 *frame, prach_map_t *prachmap)
{ // 前导发送

    // 定义一个zi帧，帧中有多少个子帧，哪个子帧有prach occasion时机
    u16 occasion_counter = 0;
    u16 final_trans_occasion = 0;
    u8 l[100] = {0};

    u16 MAX_SSB_index = get_max_SSB_RSRP(&ra->rach_configdedicated);
    final_trans_occasion = ssb_to_ro_list(ra, l, MAX_SSB_index, prachmap); // SSB到随机接入时刻的映射，返回最终选择的那个接入时机

    for (int v = 0; v < 3; v++)
    { // 因为选择的接入前导索引规定了一个帧内只有3个子帧可以传前导，所以把这几个子帧标记
        if (prachmap->subframe_number[v] < subframe_NUM)
        {
            frame[prachmap->subframe_number[v]] = 1;
        }
    }
    if (ra->ra_state == msg1_trans) // 当ra的状态为消息1发送状态时，执行
    {

        u16 j;
        u16 k;
        // u16 subframe_counter = slot_division();
        u16 subframe_counter = 3; // this is choose 3 prach occasion
        for (int i = 0; i < subframe_NUM; i++)
        {
            if (frame[subframe_counter] == 1)
            {
                for (j = 0; j < (OFDM_per_slot * 4); j++)
                { // 设定的子载波间隔为120khz，则一个子帧内有4个时西，一个shixi内有14个ofdm符号
                    if (l[j] == 1)
                    {
                        for (k = 0; k < prachmap->msg1_FDM; k++)
                        {
                            ++occasion_counter;
                            if (occasion_counter == final_trans_occasion)
                            { // 确定发送前导的位置

                                ra->prach_resources.t_id = j / OFDM_per_slot;
                                ra->prach_resources.s_id = j % OFDM_per_slot;
                                ra->prach_resources.f_id = k;
                                ra->ra_rnti = get_ra_rnti(ra);
                                // 发送前导的函数
                                ra->receivedTargerPower = get_power_prach(ra);
                                u8 preamble_result = send_preamble();
                                if (preamble_result == 0)
                                {
                                    LOG("send preamble success!\n");
                                    prachResources[final_trans_occasion].isOccupied = 1;
                                    ra->ra_state = WAIT_RAR;
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}
/*void get_RA_window(UE_MAC_INST_t *mac){ //2 slot one RA_window
    RA_config_t *ra=&mac->ra;
    msg1_SubcarrierSpacing scs=ra->scs;
    ra->RA_window_cnt=2;
}*/

void *randomaccessProcedure(void *arg)
{

    for (int i = 0; i < MAX_PRACH_OCCASIONS; i++)
    {
        prachResources[i].index = i;
        prachResources[i].isOccupied = 0;
    }
    module_id_t mod_id;
    prach_map_t *prachmap = (prach_map_t *)malloc(sizeof(prach_map_t));
    if (prachmap == NULL)
    {
        fprintf(stderr, "prachmap configuration is NULL\n");
        pthread_exit(NULL); // 退出线程
    }
    MacRarContext *mac_rar_rec = (MacRarContext *)malloc(sizeof(MacRarContext));
    if (mac_rar_rec == NULL)
    {
        fprintf(stderr, "mac_pdu configuration is NULL\n");
        pthread_exit(NULL); // 退出线程
    }

    UE_MAC_INST_t *mac = (UE_MAC_INST_t *)malloc(sizeof(UE_MAC_INST_t));
    if (mac == NULL)
    {
        fprintf(stderr, "Failed to get MAC instance\n");
        pthread_exit(NULL); // 退出线程
    }
    RA_config_t *ra = &mac->ra;
    if (ra == NULL)
    {
        // mac->ra might not be initialized
        fprintf(stderr, "RA configuration is NULL\n");
        pthread_exit(NULL); // 退出线程
    }
    PRACH_RESOURCES_t *prach_resources = &ra->prach_resources;
    if (prach_resources == NULL)
    {
        // ra->prach_resources might not be initialized
        fprintf(stderr, "PRACH resources is NULL\n");
        pthread_exit(NULL); // 退出线程
    }
    init_RA(ra, prach_resources);
    ra->ra_state = RA_UE_IDLE; //
    int backoffTime = 0;
    // get_prach_resources(mod_id);
    u16 trans_preamble_num = 0;                        // preamble transmitted number
    u16 *frame = malloc(sizeof(u16) * (subframe_NUM)); // choose ra subframe
    if (frame == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        // 处理内存分配失败的情况
        pthread_exit(NULL); // 退出线程
    }
    for (int i = 0; i < subframe_NUM; i++)
    {
        frame[i] = 0; // init
    }
    prach_resources->RA_PREAMBLE_TRANSMISSION_COUNTER = 0;
    while (prach_resources->RA_PREAMBLE_TRANSMISSION_COUNTER < ra_preambletransMax)
    {
        // send_preamble_flag = 1;
        ra->ra_state = msg1_trans;
        msg1_transmitted(ra, frame, prachmap);

        if (ra->ra_state == WAIT_RAR)
        {
            LOG("waiting rar from leo\n");
            ra->ra_state = receive_rar;
            // break;
        }
        long time_msg1 = getCurrentMicroseconds();
        LOG("msg1 send success time and rar window open time: %ld\n", time_msg1);
        ra->ra_ResponseWindow = ra_responsewindow;
        long opentime = ra->ra_ResponseWindow * subframe_length; ////开启响应接收窗口  two subframe 单位:us
        long endtime = time_msg1 + opentime;
        pthread_mutex_lock(&lock);
        LOG("Thread ue: Condition met, signaling thread leo to proceed.\n");
        pthread_cond_signal(&cond2);
        pthread_cond_wait(&cond1, &lock);
        pthread_mutex_unlock(&lock);
        long time2 = getCurrentMicroseconds();
        LOG("Thread ue: Resuming execution after thread leo has met its condition. \n");

        LOG(" rar prepare thread time: %ld\n", time2 - time_msg1);
        while (time2 < endtime)
        {
            mac_rar_rec = get_rar();

            if (mac_rar_rec == NULL)
            {
                mac_rar_rec = get_rar(); // zhouqixing caozuo
            }
            else
            {
                time_msg1 = getCurrentMicroseconds();
                LOG("rar received at us: %ld\n", time_msg1);
                break;
            }
            time_msg1 = getCurrentMicroseconds();
        }

        if (ra->ra_state == receive_rar)
        {

            listenRARWindow(ra, mac_rar_rec);
            if (rar_received_flag == -1)
            {
                prach_resources->RA_PREAMBLE_TRANSMISSION_COUNTER++;
            }
        }
        if (ra->ra_state == RA_SUCCEESED)
        {
            printf("random access successfully!\n");
            break;
        }
        if (prach_resources->RA_PREAMBLE_TRANSMISSION_COUNTER == ra_preambletransMax)
        {
            ra->ra_state == RA_FAILED;
            printf("UE transmitted Random Access Preamble failed\n");
        }

        /* if(received_rar()){

             return 0;
         }else{
             //calculate backofftime
             printf("RAR not received, UE backs off for %d ms\n", backoffTime);
             wait(backoffTime); //BI
         }*/
    }
    // LOG("Random Access procedure failed after maximum preamble transmissions\n");

    free(prachmap);
    free(mac_rar_rec);
    free(mac);

    // pthread_mutex_lock(&lock);
    return NULL;
}

u16 prach_occasion_config(prach_map_t *prachmap, u8 *l, u16 *occasion_subframe_num, u16 *occasion_frame_num)
{ // prach occasion configuration
    // u8 l[100];  //time index    l is all slots
    // l_time=l;
    u8 l_num = 0;
    u8 index = 0;
    // u16 occasion_subframe_num;
    // u16 occasion_frame_num;
    if (prachmap == NULL)
    {
        fprintf(stderr, "Error: prachmap is a NULL pointer\n");
        return (u16)-1; // 或者其他适当的错误处理
    }
    prachmap->config_index = prach_ConfigIndex; // 3gpp 38.211协议中根据选择的这个索引设定下面的参数

    prachmap->preamble_format = 0xB1;
    prachmap->x = 1;
    prachmap->y = 0;
    memcpy(prachmap->subframe_number, (u8[]){3, 5, 7}, sizeof(prachmap->subframe_number));

    prachmap->start_symbol = 2;
    prachmap->N_P_slots_per_subframe = 1;
    prachmap->N_td_per_slot = 6;
    prachmap->N_dur = 2;
    prachmap->msg1_FDM = msg1_fdm;
    prachmap->msg1_FrequencyStart = MSG1_FrequencyStart;
    prachmap->preambleTransMax = ra_preambletransMax;
    prachmap->powerRampingStep = powerRampingSTEP;
    // prachmap->ra_ResponseWindow=ra_responsewindow;
    for (int i = 0; i < prachmap->N_P_slots_per_subframe; i++)
    {
        for (int j = 0; j < prachmap->N_td_per_slot; j++)
        {

            index = prachmap->start_symbol + j * prachmap->N_dur + 14 * i; // 计算时域的位置
            l_num++;
            l[index] = 1;
            /*
            if(symbol==index){
                start_symbol_index = index;
                break;
            }*/
        }
    }
    //*occasion_subframe_num=prachmap->msg1_FDM*prachmap->N_P_slots_per_subframe;  //子帧上的prach occasion数量
    //*occasion_frame_num=*occasion_subframe_num*sizeof(prachmap->subframe_number);  //一个帧上的prach occasion数量
    // prach_occasion_id = prachmap->msg1_FDM * start_symbol_index + freq_offset ;

    return l_num;
}

u16 ssb_to_ro_list(RA_config_t *ra, u8 *l, u16 MAX_SSB_index, prach_map_t *prachmap)
{

    u16 occasion_subframe_num = 0;
    u16 occasion_frame_num = 0;
    u16 l_num = prach_occasion_config(prachmap, l, &occasion_subframe_num, &occasion_frame_num);
    ra->rach_configdedicated.ssb_per_prach_occasion = SSB_per_prach_occasion;
    float ssb_per_prach_occasion = ra->rach_configdedicated.ssb_per_prach_occasion;
    u16 temp_Demandoccasionnums; // 传输ssb需要的prach occasion数量
    u16 targetoccasion;
    // u16 MAX_SSB_INDEX=occasion_frame_num * SSB_per_prach_occasion ; //一个帧中最多传输的SSB数量
    if (ssb_per_prach_occasion < 1)
    {
        if (ssb_per_prach_occasion == 0)
        {
            fprintf(stderr, "Error: Division by zero\n");
            // 处理错误，例如返回错误代码或抛出异常
            return -1;
        }
        temp_Demandoccasionnums = SSB_num / ssb_per_prach_occasion; // 总的需要的prachoccasion数量
    }
    else
    {
        temp_Demandoccasionnums = SSB_num / ssb_per_prach_occasion;
        temp_Demandoccasionnums = temp_Demandoccasionnums == 0 ? 1 : temp_Demandoccasionnums; // 避免为0
    }
    u16 PrachPeriod = prachmap->y == 0 ? 1 : prachmap->x / prachmap->y; // 周期
    // 选择occasion
    if (ssb_per_prach_occasion < 1)
    {
        targetoccasion = (u16)(MAX_SSB_index / ssb_per_prach_occasion) + rand() % (u16)(1 / ssb_per_prach_occasion); // 加一个随机数减小多个用户同时发送接入请求的概率
    }
    else
    {
        targetoccasion = MAX_SSB_index / ssb_per_prach_occasion; // 选择前导具体发送的那个prachoccasion
        targetoccasion = targetoccasion == 0 ? 1 : targetoccasion;
    }
    // prachResources[targetoccasion].isOccupied = 1;
    return targetoccasion;
}
int ZC_preamble(float zc_sequence[][PREAMBLE_LENGTH])
{ // 64 preamble one cell
    // int v=63;  //
    int u = 1;
    // float zc_sequence[v][PREAMBLE_LENGTH];
    if (zc_sequence == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }
    for (int i = 0; i < total_preamble_num; i++)
    {
        for (int n = 0; n < PREAMBLE_LENGTH; n++)
        {
            int x = (n + i * NCS) % PREAMBLE_LENGTH;
            zc_sequence[i][n] = cos(M_PI * (u * (x * (x + 1))) / PREAMBLE_LENGTH);
        }
    }
    return 0;
}
int send_preamble()
{
    // 先选择zc序列中的第一个试试
    float(*zc_sequence)[PREAMBLE_LENGTH] = malloc(total_preamble_num * sizeof(float) * PREAMBLE_LENGTH);
    if (zc_sequence == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }
    u16 result = ZC_preamble(zc_sequence);
    if (result != 0)
    {
        free(zc_sequence); // 释放之前分配的内存
        return result;     // 这里假设ZC_preamble返回非0表示错误
    }
    zc_sequence[1];
    int CP_LENGTH = 16;
    int GI_LENGTH = 16;
    // 在前导码前添加循环前缀
    int total_length = PREAMBLE_LENGTH + CP_LENGTH + GI_LENGTH;
    float preamble_final[total_length];
    for (int i = 0; i < CP_LENGTH; i++)
    {
        preamble_final[i] = zc_sequence[0][PREAMBLE_LENGTH - CP_LENGTH + i];
    }
    for (int i = 0; i < PREAMBLE_LENGTH; i++)
    {
        preamble_final[CP_LENGTH + i] = zc_sequence[0][i];
    }
    for (int i = 0; i < GI_LENGTH; i++)
    {
        preamble_final[CP_LENGTH + GI_LENGTH + i] = 0;
    }
    // add UDP套接字先发送
    return 0;
}
/*bool received_rar(MacRarContext *mac_rar_ctx,RA_config_t *ra){
    // 初始化MAC层RAR上下文
   // MacRarContext *mac_rar_ctx;
    //mac_rar_ctx->rar_buffer_size = 0;
    //mac_rar_ctx->backoff_slot = 0;
    RarMessage *received_rar;
    handle_rar(mac_rar_ctx,ra);
    //random access
    //LOG("UE received Random Access Reponse\n");

}*/
u16 handle_rar(MacRarContext *mac_rar_ctx, RA_config_t *ra)
{                        // 处理接收的rar消息
    u8 n_subPDUs = 0;    // number of RAR payloads
    u8 n_subheaders = 0; // number of MAC RAR subheaders
    u8 *dlsch_buffer = mac_rar_ctx->rar_buffer;
    NR_RA_HEADER_RAPID *rar_header = (NR_RA_HEADER_RAPID *)dlsch_buffer;
    NR_MAC_RAR *rar = (NR_MAC_RAR *)(dlsch_buffer + 1);
    ra->preamble_index = 21;
    u8 preamble_index = ra->preamble_index;
    while (1)
    {
        n_subheaders++;
        if (rar_header->T == 1)
        {
            n_subPDUs++;
            LOG("[UE ][RAPROC] Got RAPID RAR subPDU\n");
        }
        else
        {
            ra->RA_backoff_indicator = table_7_2_1[((NR_RA_HEADER_BI *)rar_header)->BI]; // ms
            ra->RA_BI_found = 1;
            LOG("[UE ][RAPROC] Got BI RAR subPDU %d ms\n", ra->RA_backoff_indicator);
            if (((NR_RA_HEADER_BI *)rar_header)->E == 1)
            {
                rar_header += sizeof(NR_RA_HEADER_BI);
                continue;
            }
            else
            {
                break;
            }
        }
        if (rar_header->RAPID == preamble_index)
        {

            rar = (NR_MAC_RAR *)(dlsch_buffer + n_subheaders + (n_subPDUs - 1) * sizeof(NR_MAC_RAR));
            ra->RA_RAPID_found = 1;
        }
        if (rar_header->E == 0)
        {
            // LOG_W(NR_MAC,"[UE %d][RAPROC][%d.%d] Received RAR preamble (%d) doesn't match the intended RAPID (%d)\n", mod_id, frame, slot, rarh->RAPID, preamble_index);
            break;
        }
        else
        {
            rar_header += sizeof(NR_MAC_RAR) + 1;
        }
    }
    if (ra->RA_RAPID_found)
    {
        UL_grant_t rar_grant;
        memset(&rar_grant, 0, sizeof(UL_grant_t));
        u8 tpc_command;
        u8 csi_req;
        // TA command
        NR_UL_TIME_ALIGNMENT_t *ul_time_alignment = (NR_UL_TIME_ALIGNMENT_t *)malloc(sizeof(NR_UL_TIME_ALIGNMENT_t));
        memset(ul_time_alignment, 0, sizeof(NR_UL_TIME_ALIGNMENT_t));
        if (ul_time_alignment == NULL)
        {
            fprintf(stderr, "Failed to get MAC instance\n");
            pthread_exit(NULL); // 退出线程
        }
        const int ta = rar->TA2 + (rar->TA1 << 5);
        ul_time_alignment->ta_command = 31 + ta;
        ul_time_alignment->ta_total = ta;
        ul_time_alignment->ta_apply = true;
        LOG("received TA command %d\n", 31 + ta);
        // CSI
        csi_req = (u8)(rar->UL_GRANT_4 & 0x01);
        // TPC
        tpc_command = (u8)((rar->UL_GRANT_4 >> 1) & 0x07);
        switch (tpc_command)
        {
        case 0:
            ra->Msg3_TPC = -6;
            break;
        case 1:
            ra->Msg3_TPC = -4;
            break;
        case 2:
            ra->Msg3_TPC = -2;
            break;
        case 3:
            ra->Msg3_TPC = 0;
            break;
        case 4:
            ra->Msg3_TPC = 2;
            break;
        case 5:
            ra->Msg3_TPC = 4;
            break;
        case 6:
            ra->Msg3_TPC = 6;
            break;
        case 7:
            ra->Msg3_TPC = 8;
            break;
        }
        // MCS
        rar_grant.mcs = (u8)(rar->UL_GRANT_4 >> 4);
        // time alloc
        rar_grant.Msg3_t_alloc = (u8)(rar->UL_GRANT_3 & 0x0f);
        // frequency alloc
        rar_grant.Msg3_f_alloc = (u16)((rar->UL_GRANT_3 >> 4) | (rar->UL_GRANT_2 << 4) | ((rar->UL_GRANT_1 & 0x03) << 12));
        // frequency hopping
        rar_grant.freq_hopping = (u8)(rar->UL_GRANT_1 >> 2);
    }
    return ra->RA_RAPID_found;
}
void listenRARWindow(RA_config_t *ra, MacRarContext *mac_rar_ctx)
{

    rar_received_flag = 0;

    if (handle_rar(mac_rar_ctx, ra) == 1)
    {
        // rarstate->rarreceived=true;
        LOG("RAR handle success\n");
        ra->ra_state = RA_SUCCEESED;
    }
    else
    {
        // rarstate->rarreceived=false;
        rar_received_flag = -1;
        // ra->prach_resources.RA_PREAMBLE_TRANSMISSION_COUNTER++;
    }
}
MacRarContext *get_rar()
{
    MacRarContext *ctx = malloc(sizeof(MacRarContext));
    if (ctx == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for MacRarContext\n");
        exit(EXIT_FAILURE); // 或者使用其他适当的错误处理
    }
    memcpy(ctx->rar_buffer, mac_rar_ctx.rar_buffer, mac_rar_ctx.rar_buffer_size);
    ctx->rar_buffer_size = mac_rar_ctx.rar_buffer_size;
    return ctx;
}
