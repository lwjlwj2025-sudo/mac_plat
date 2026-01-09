#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include <math.h>
#include <unistd.h>
#include "ra_config.h"
#include "config.h"
#include "mac_process.h"
#include "timer_queue.h"
#include "CONSTANTS.h"
#include "globals.h"
#include "package.h"
#include "info_process.h"
#include "resource_apply.h"
// #include "App_queue.h"

int online_detect_start_en = 1;
frame_info *frame_t = NULL;
u16 package_totalNum_Array[BBU_UE_MAXNUM] = {0};
u16 pckage_recvNum_Array[BBU_UE_MAXNUM] = {0};
u16 pckage_datalen_Array[BBU_UE_MAXNUM] = {0};
u8 SELF_TRANS_EN = 1; // 业务信道对齐使能
u8 SCH_SEND_EN = 0;   // SCH发送使能
u8 CCH_SEND_EN = 0;   // CCH发送使能
UserInfo *UserInfo_PtrArray[BBU_UE_MAXNUM];
UserInfo_Block wave_ueser_mapPtrArray[WAVE_NUM][PER_WAVE_MAX_NUM];
// 在线时间登记表
int Online_Time_Table[BBU_UE_MAXNUM] = {0};

// 记录各个虚波位的CCH_RB分出去的数量
u16 occupied_dlcch_rbnum[WAVE_NUM];

u16 cch_rb_id[RB_TOTALNUM] = {0};
u16 sch_rb_id[SSLOT_TOTALNUM][RB_TOTALNUM] = {0};
int access_ue_count = 0;
int last_detect_time = 0; // 上一次用户在线状态检测的时间，初始化为0

// 记录还没有分出去的业务RB的最小值，取值250~999，=1000时说明没资源了
u16 DL_min_idleRB = 250;
// 非驻留业务信道资源可用的业务RB的最小值
u16 DL_min_flash_available_idleRB = 250;
// 记录还没有分出去的业务RB的最小值，取值250~999，=1000(SCH_RB_NUM :业务信道的RB数量)时说明没资源了
u16 UL_min_idleRB = 250;
// 非驻留业务信道资源可用的业务RB的最小值
u16 UL_min_flash_available_idleRB = 250;
// 虚波位轮询，这玩意需要跟物理层商量一下咋更新
// u16 poll_wave_index = 0;

// 空闲RB队列管理列表，储存150个队列（queue）的指针，对应150个波位的空闲RB队列，每个队列上限容纳250个RB
App_queue *UL_Idle_CCH_RB_Queue_List[WAVE_NUM];
App_queue *DL_Idle_CCH_RB_Queue_List[WAVE_NUM];
App_queue *UL_Idle_SCH_RB_Queue_List[WAVE_NUM];
App_queue *DL_Idle_SCH_RB_Queue_List[WAVE_NUM];
App_queue *DL_BusinessPackage_Queue_List[WAVE_NUM];
App_queue *DL_ControlPackage_Queue_List[WAVE_NUM];
App_queue *UE_data_recv_Queue_List[BBU_UE_MAXNUM];

App_queue *DL_data_buffer_Queue_List[BBU_UE_MAXNUM];
cvector *DL_data_buffer_Cvector_List[BBU_UE_MAXNUM];
/************/

SCH_RB_occupiednum *occupiednum_dlsch_rbnum[WAVE_NUM];
SCH_RB_idlenum *SCH_RB_idlenum_PtrArray[WAVE_NUM];

// 记录每个虚波位接入了多少用户，一边快速的将信息填入wave_ueser_mapPtrArray
u8 per_wave_usernum[WAVE_NUM];
Mac_to_PHY Mac_to_PHY_data[PER_WAVE_MAX_NUM] = {0};
Mac_to_PHY_rec RB_Reception_Info[PER_WAVE_MAX_NUM] = {0};

// 开的char类型数组的大小为虚波位用户接满，且业务信息全为最大的情况
u8 Mac_to_PHY_send_frame_buffer[PER_WAVE_MAX_NUM * (1031 + 1 + CCH_DATA_LENGTH) + 1] = {0};
u8 Mac_to_PHY_rec_frame_buffer[PER_WAVE_MAX_NUM * 8 + 1] = {0};
/****************/

void slot_division()
{
    // frame_info *frame_t = (frame_info *)malloc(sizeof(frame_info));
    int superframe_num;         // 窄波束复帧号
    int frame_num;              // 窄波束帧号
    int subframe_num;           // 窄波束子帧号
    int slot_num;               // 窄波束时隙号  0-29为标准DATA时隙号，-1为保护时间
    int slot_type;              // 时隙类型指示
    long SuperFrame_TimeLength; // 复帧（超帧）内的时间间长度，<1s     单位：微秒
    long Frame_TimeLength;      // 窄波束帧内时间长度,<100ms   单位：微秒
    long subFrame_TimeLength;   // 子帧内时间长度，<0.5ms    单位：微秒
    long Slot_pastLength;       // 时隙内距离时隙起始点已过去的时间长度，<16ms

    u8 data_buff[256] = {0};
    u16 data_len = 0;

    // 获取当前复帧号
    superframe_num = getCurrentSecond();

    // 获取复帧内部时间长度
    SuperFrame_TimeLength = getCurrentMicroseconds();

    // 计算帧号
    frame_num = (int)(SuperFrame_TimeLength / frame_length);

    // 计算帧内的时间长度
    Frame_TimeLength = SuperFrame_TimeLength - (long)(frame_num * frame_length);

    // 计算子帧号
    subframe_num = (int)(Frame_TimeLength / subframe_length);

    // 计算子帧内部时间长度
    subFrame_TimeLength = Frame_TimeLength - (long)(subframe_num * subframe_length);

    // 若子帧内时间长度在保护时间外，则计算时隙号，否则时隙类型定义为保护时间
    if (subFrame_TimeLength >= guard_time)
    {
        // 计算时隙号
        slot_num = (int)((subFrame_TimeLength - guard_time) / slot_length);

        // 判断时隙类型
        if (slot_num == 0)
        {
            slot_type = CCH_SLOT;
        }
        else if (slot_num == 1 || slot_num == 2 || slot_num == 3)
        {
            slot_type = SCH_SLOT;
        }
        else
        {
            // 时隙号计算有误
            slot_type = RES_SLOT;
            LOG("slot_num calculate error");
        }

        // 计算时隙内部时间长度
        Slot_pastLength = subFrame_TimeLength - guard_time - slot_num * slot_length;

        // 判断当前是否位于DATA时隙的可发送区间
        if (Slot_pastLength <= slot_length - minimumTimeAdvance_to_PHY)
        {
            // LOG("pos in send area");
            switch (slot_type)
            {
            case CCH_SLOT:
                // 发送控制信道消息 CCH

                /* 发送区 */
                // cch_pk_send();

                break;

            case SCH_SLOT:
                // 发送共享信道消息 SCH

                /* 发送区 */
                // sch_pk_send(slot_num);

                break;

            case RES_SLOT:
                // 发生错误，MAC-U目前没有RES时隙类型
                LOG("slot_type error");
                break;

            default:
                break;
            }
            // 计算下一次进入本函数的等待周期
            // 获取复帧内部时间长度
            SuperFrame_TimeLength = getCurrentMicroseconds();

            // 计算帧号
            frame_num = (int)(SuperFrame_TimeLength / frame_length);

            // 计算帧内的时间长度
            Frame_TimeLength = SuperFrame_TimeLength - (long)(frame_num * frame_length);

            // 计算子帧号
            subframe_num = (int)(Frame_TimeLength / subframe_length);

            // 计算子帧内部时间长度
            subFrame_TimeLength = Frame_TimeLength - (long)(subframe_num * subframe_length);

            // 计算时隙号
            slot_num = (int)((subFrame_TimeLength - guard_time) / slot_length);

            // 计算时隙内部时间长度
            Slot_pastLength = subFrame_TimeLength - guard_time - slot_num * slot_length;

            // 下一次进入本函数的等待周期
            slot_division_period = slot_length - Slot_pastLength;
        }
        else // DATA有效发送区之外，不发包，等待下一时隙
        {
            // LOG("pos in unable send area");
            // 计算下一次进入本函数的等待周期
            // 获取复帧内部时间长度
            SuperFrame_TimeLength = getCurrentMicroseconds();

            // 计算帧号
            frame_num = (int)(SuperFrame_TimeLength / frame_length);

            // 计算帧内的时间长度
            Frame_TimeLength = SuperFrame_TimeLength - (long)(frame_num * frame_length);

            // 计算子帧号
            subframe_num = (int)(Frame_TimeLength / subframe_length);

            // 计算子帧内部时间长度
            subFrame_TimeLength = Frame_TimeLength - (long)(subframe_num * subframe_length);

            // 计算时隙号
            slot_num = (int)((subFrame_TimeLength - guard_time) / slot_length);

            // 计算时隙内部时间长度
            Slot_pastLength = subFrame_TimeLength - guard_time - slot_num * slot_length;

            // 下一次进入本函数的等待周期
            slot_division_period = slot_length - Slot_pastLength;
        }
    }
    else
    {
        slot_num = -1;                                           // 保护时间的时隙号为-1
        slot_type = GT_SLOT;                                     // 保护时间
        slot_division_period = guard_time - subFrame_TimeLength; // 下一次进入本函数的等待周期
        LOG("jinrubaohujiange");
    }
    frame_t->frame_num = frame_num;
    frame_t->sub_frame_num = subframe_num;
    frame_t->slot_num = slot_num;
    LOG("frame_t->frame_num: %d, frame_t->sub_frame_num: %d, frame_t->slot_num: %d",
        frame_t->frame_num, frame_t->sub_frame_num, frame_t->slot_num); // 每个帧里面是200个子帧，每个子帧是4个时隙
    // return frame_t;
}
// 更新令牌桶中的令牌数量
void update_tokens(Logicalchannel *channel, u32 T)
{ // T:ms
    u8 tokens_to_add = channel->PBR * (T / 1000);
    if (channel->Bj + tokens_to_add <= channel->BSD * channel->PBR)
    {
        channel->Bj += tokens_to_add;
    }
    else
    {
        channel->Bj = channel->BSD * channel->PBR;
    }
}
// 冒泡排序
void sortLevel_1(Logicalchannel channels[], int num_channels)
{
    Logicalchannel p;
    int index = 0;
    for (index = 0; index < num_channels; index++)
    {
        for (int j = 0; j < num_channels - index - 1; j++)
        {
            if (channels[j].priority > channels[j + 1].priority)
            {
                p.Bj = channels[j].Bj;
                p.BSD = channels[j].BSD;
                p.PBR = channels[j].PBR;
                p.priority = channels[j].priority;
                channels[j].Bj = channels[j + 1].Bj;
                channels[j].BSD = channels[j + 1].BSD;
                channels[j].PBR = channels[j + 1].PBR;
                channels[j].priority = channels[j + 1].priority;
                channels[j + 1].Bj = p.Bj;
                channels[j + 1].BSD = p.BSD;
                channels[j + 1].PBR = p.PBR;
                channels[j + 1].priority = p.priority;
            }
        }
    }
}
// 先经过令牌桶算法然后复用，能从每个逻辑信道桶中流出来的才能复用
u16 allocateResources(Logicalchannel channels[], int num_channels, RLC_SDU sdus[][MAX_SDUS_PER_CHANNEL], int num_sdus_per_channel[], p_sdu_resource p_sdu[MAC_SDU_NUM], App_queue *pack_queue)
{
    // initial logicalchannel fangzaiwaimian

    sortLevel_1(channels, 3);
    App_queue *buffer_sdu = (App_queue *)p_sdu;

    for (int t = 0; t < MAC_SDU_NUM; t++)
    {
        for (int i = 0; i < num_channels; i++)
        {
            update_tokens(&channels[i], 500); // 每经过一个子帧时间间隔（T=500us）就向令牌桶中注入新令牌
            for (int j = 0; j < num_sdus_per_channel[i]; j++)
            {
                if (channels[i].Bj > 0 && channels[i].Bj >= sdus[i][j].size)
                {
                    channels[i].Bj -= sdus[i][j].size;
                    if (App_queue_size(buffer_sdu) < MAC_PDU_SIZE && (MAC_PDU_SIZE - App_queue_size(buffer_sdu)) >= sdus[i][j].size)
                    {
                        App_queue_push(buffer_sdu, sdus[i][j].data, sdus[i][j].size);
                    }
                    else
                    {
                        return -1;
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }
    multiplex_data(pack_queue, p_sdu);
    fill_remaining_space(pack_queue, MAC_PDU_SIZE);
    return 0;
}
// p_sdu_resource p_sdu[MAC_SDU_NUM];
u16 multiplex_data(App_queue *pack_queue, p_sdu_resource p_sdu[MAC_SDU_NUM])
{ // 所有sdu都加上子头后复用
    // App_queue *pack_queue=(App_queue*)malloc(sizeof(App_queue));
    for (int sdu_idx = 0; sdu_idx < MAC_SDU_NUM; sdu_idx++)
    {
        App_queue_push(pack_queue, 0, 1); // R
        if (p_sdu[sdu_idx].datalength > 255)
        {
            App_queue_push(pack_queue, (char *)1, 1);                             // F=1
            App_queue_push(pack_queue, (char *)&(p_sdu[sdu_idx].LCID), 6);        // LCID
            App_queue_push(pack_queue, (char *)&(p_sdu[sdu_idx].datalength), 16); // L
        }
        else
        {
            App_queue_push(pack_queue, 0, 1);                                    // F=0
            App_queue_push(pack_queue, (char *)&(p_sdu[sdu_idx].LCID), 6);       // LCID
            App_queue_push(pack_queue, (char *)&(p_sdu[sdu_idx].datalength), 8); // L
        }
        App_queue_push(pack_queue, p_sdu[sdu_idx].DATA, sizeof(p_sdu[sdu_idx].DATA) / sizeof(p_sdu[sdu_idx].DATA[0]));
    }
}
// 填充剩余的MAC PDU空间 padding
void fill_remaining_space(App_queue *packpdu_queue, u8 PDU_SIZE)
{
    u8 remaining_space = PDU_SIZE - App_queue_size(packpdu_queue);
    u8 *buffer = (u8 *)packpdu_queue;
    if (remaining_space > 0)
    {
        for (int i = 0; i < remaining_space; i++)
        {
            buffer[i] = 0;
        }
    }
}
void init_Logical(Logicalchannel *channel)
{
    channel->Bj = 0;
    channel->BSD = 0; // ms
    channel->PBR = 0; // kbps
    // channel->priority = 0;
    channel->logicalchannelSR_mask = false;
    channel->waitingdatabuffer = 0;
}
u16 demultiplex_data(u8 *bitStream)
{ // msg from rlc
    u8 bitend = sizeof(bitStream) / sizeof(bitStream[0]);
    u8 bitnum = 0, bytenum = 0, sduindex = 0;
    u8 F, LCID;
    u16 L;
    mac_SDU **data_SDU = NULL; // 初始化为 NULL
    // mac_SDU *data_SDU[]
    while (bitnum != bitend)
    {
        u8 LCID = getParamFromBitstream(bitStream, bitnum + 2, 6);

        if (LCID == 0)
        {
            F = getParamFromBitstream(bitStream, bitnum + 1, 1);
            bitnum += 8;
            if (F == 0)
            {
                L = getParamFromBitstream(bitStream, bitnum, 8);
                bitnum += 8;
            }
            else
            {
                L = getParamFromBitstream(bitStream, bitnum, 16);
                bitnum += 16;
            }
            bytenum = bitnum >> 3;
            mac_ccch_SDU *de_SDU = (mac_ccch_SDU *)malloc(sizeof(mac_ccch_SDU));
            de_SDU->ccch_length = L;
            memcpy(de_SDU->DATA, &bitStream[bytenum], L);
            bitnum += L * 8;
        }
        else if (LCID >= 1 && LCID <= 32)
        {

            F = getParamFromBitstream(bitStream, bitnum + 1, 1);
            bitnum += 8;
            if (F == 0)
            {
                L = getParamFromBitstream(bitStream, bitnum, 8);
                bitnum += 8;
            }
            else
            {
                L = getParamFromBitstream(bitStream, bitnum, 16);
                bitnum += 16;
            }
            bytenum = bitnum >> 3;
            // 动态分配数组
            sduindex++; // 增加一个元素
            // 重新分配足够的空间以容纳新元素
            data_SDU = realloc(data_SDU, sduindex * sizeof(mac_SDU *));
            if (data_SDU == NULL)
            {
                fprintf(stderr, "Memory allocation failed for data_SDU array.\n");
                exit(EXIT_FAILURE);
            }
            // 为新的元素分配内存
            data_SDU[sduindex - 1] = malloc(sizeof(mac_SDU));
            if (data_SDU[sduindex - 1] == NULL)
            {
                fprintf(stderr, "Memory allocation failed for mac_SDU at index %d.\n", sduindex - 1);
                // 清理已分配的内存
                for (int i = 0; i < sduindex - 1; i++)
                {
                    free(data_SDU[i]);
                }
                free(data_SDU);
                exit(EXIT_FAILURE);
            }
            data_SDU[sduindex - 1]->sdusize = L;
            memcpy(data_SDU[sduindex - 1]->DATA, &bitStream[bytenum], L);
            bitnum += L * 8;
        }
    }
    return 0;
}

u16 getParamFromBitstream(u8 *bitStream, u16 bitPos, u8 numBits)
{
    u16 value = 0;
    u16 bytePos = bitPos >> 3;    // 计算字节偏移量
    u16 bitOffset = bitPos & 0x7; // 获取剩余的未对齐的位数
    if (numBits + bitOffset > 8)
    { // 如果跨越多个字节
        u8 remainingBitsInLastByte = 8 - bitOffset;
        u8 bitsToReadFirstByte = numBits < remainingBitsInLastByte ? numBits : remainingBitsInLastByte;
        value = (bitStream[bytePos] << bitOffset) & ((1 << bitsToReadFirstByte) - 1);
        numBits -= bitsToReadFirstByte;
        bytePos++;
        while (numBits >= 8)
        {
            value |= (bitStream[bytePos++] << 0) & 0xFF;
            numBits -= 8;
        }
        if (numBits > 0)
        {
            value |= (bitStream[bytePos] >> (8 - numBits)) & ((1 << numBits) - 1);
        }
    }
    else
    { // 如果在同一个字节内
        value = (bitStream[bytePos] >> (8 - numBits - bitOffset)) & ((1 << numBits) - 1);
    }
    return value;
}
u16 cal_RB_carry_capacity(u16 MCS)
{
    u16 per_RB_ByteNum;

    // 获取当前用户每个RB可以承载的字节数
    switch (MCS)
    {
    // case 0:
    //     per_RB_ByteNum = 40;
    //     break;
    case 1:
        per_RB_ByteNum = 7;
        break;
    case 2:
        per_RB_ByteNum = 18;
        break;
    case 3:
        per_RB_ByteNum = 25;
        break;
    case 4:
        per_RB_ByteNum = 27;
        break;
    case 5:
        per_RB_ByteNum = 32;
        break;
    case 6:
        per_RB_ByteNum = 46;
        break;
    case 7:
        per_RB_ByteNum = 55;
        break;
    default:
        break;
    }

    return per_RB_ByteNum;
}

void MAC_to_PHY_test()
{
    u8 user_index;

    for (user_index = 0; user_index < 2; user_index++)
    {
        // 接入一个假用户，下边的都是下行业务需要的信息
        wave_ueser_mapPtrArray[1][user_index].State = 1;
        wave_ueser_mapPtrArray[1][user_index].UE_Level = 1;
        wave_ueser_mapPtrArray[1][user_index].U_RNTI = user_index;
        wave_ueser_mapPtrArray[1][user_index].MCS = 5;
        wave_ueser_mapPtrArray[1][user_index].User_DataQueue_ptr = DL_data_buffer_Cvector_List[user_index];

        UserInfo_PtrArray[user_index]->state = 1;
        UserInfo_PtrArray[user_index]->U_RNTI = user_index;
        memcpy(UserInfo_PtrArray[user_index]->UE_MAC_addr.value, own_address.value, MY_MAC_ADDR_BYTE_NUM);
        // UserInfo_PtrArray[ue_local_index]->UE_MAC_addr                                      = ue_addr;
        UserInfo_PtrArray[user_index]->BeamPositionIndex = 1;
        UserInfo_PtrArray[user_index]->wave_user_index = user_index;
        UserInfo_PtrArray[user_index]->IPv4 = 0;
        UserInfo_PtrArray[user_index]->CCH_SlotIndex.slot0_uplink_en = 1;
        UserInfo_PtrArray[user_index]->CCH_SlotIndex.slot0_downlink_en = 1;
        UserInfo_PtrArray[user_index]->CCH_FrequencyPoint.slot0_uplink_FrequencyPoint = user_index * CCH_RB_FREQ_POINT_ARRAY_SIZE;   // CCH只有一个时隙，因此RB_ID与时隙中RB的序号值相同
        UserInfo_PtrArray[user_index]->CCH_FrequencyPoint.slot0_downlink_FrequencyPoint = user_index * CCH_RB_FREQ_POINT_ARRAY_SIZE; // 控制面是只有一个信道，250个PRB，要分给十个用户

        UserInfo_PtrArray[user_index]->DL_MCS = 5;
        UserInfo_PtrArray[user_index]->UL_MCS = 5;

        LOG("接入了一个用户，User_DataQueue_ptr = %x, slot0_uplink_FrequencyPoint = %d\n", wave_ueser_mapPtrArray[1][user_index].User_DataQueue_ptr, UserInfo_PtrArray[user_index]->CCH_FrequencyPoint.slot0_uplink_FrequencyPoint);

        // 生成十个业务包，并插入到对应的缓存里
        u16 index;
        index = 0;
        for (index; index < 10; index++)
        {
            u8 buffer[256] = {0};
            u8 package_buff[256] = {0};
            type_msg_division_s *msg_division_frame = (type_msg_division_s *)buffer;
            msg_division_frame->CID = TYPE_MSG_DIVISION;
            msg_division_frame->Data_level = 0;
            msg_division_frame->UE_level = 2;
            msg_division_frame->Source_IP = 0;
            msg_division_frame->Dest_IP = user_index;
            msg_division_frame->Data_len = 60;

            u8 arr[60];
            int index;
            index = 0;
            for (index; index < 60; index++)
            {
                arr[index] = index;
            }
            msg_division_frame->Data[0] = arr;
            // arr next buffer
            // u16 recv_rlc_data_len = Package_Frame(0, arr, 60, buffer, sizeof(buffer));
            // memcpy(msg_division_frame->Data, recv_rlc_data, recv_rlc_data_len);
            // for (int i = 0; i < sizeof(buffer); i++)
            //{
            //     printf("%02x ", buffer[i]);
            // }
            // memset(msg_division_frame->Data, 0xFF, 60);

            u32 package_buffLen = Package_Frame(TYPE_MSG_DIVISION, buffer, sizeof(type_msg_division_s) + 60, package_buff, sizeof(package_buff)); // add head

            cvector_pushback(wave_ueser_mapPtrArray[1][user_index].User_DataQueue_ptr, package_buff, package_buffLen);
        }

        LOG("生成十个定长的业务包，并存到对应的缓存\n");
    }
}
