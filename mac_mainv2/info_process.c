#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "CONSTANTS.h"
#include "mac_process.h"
#include "globals.h"
#include "package.h"
#include "info_process.h"
// #include    "App_queue.h"
#include "App_udp.h"

void RAMSG_process(msg_all *msg) // handle rrc info
{
    // Package_RAMSG((u8 *) msg, sizeof(msg_all)); 这里面是设置包的
    type_RAMSG_s *RAMSG_frame = (type_RAMSG_s *)msg->HEAD->DATA;
    // u32 get_ue_addr = RAMSG_frame->UEMAC;
    u48 get_ue_addr;
    memcpy(get_ue_addr.value, RAMSG_frame->UEMAC.value, MY_MAC_ADDR_BYTE_NUM);
    u16 get_wave_num = RAMSG_frame->Wave_num;
    u32 get_ue_rnti = RAMSG_frame->URNTI;
    u32 get_ue_ip = RAMSG_frame->IP;
    LOG("Sat MAC-U recv RAMSG, UE MAC addr = %d\n", get_ue_addr);
    u16 ue_bbu_index = RNTI_to_LocalIndex_map(get_ue_rnti, BBU_index); // 将ue的rnti映射到基带单元
    LOG("rnti is %d,index is %d", get_ue_rnti, ue_bbu_index);
    u16 wave_locall_index = WAVE_NUM_to_LocalIndex_map(get_wave_num); // 映射用户的波束选择到基带单元

    if (App_queue_empty(UL_Idle_CCH_RB_Queue_List[ue_bbu_index]) || App_queue_empty(DL_Idle_CCH_RB_Queue_List[ue_bbu_index]))
    {
        LOG("波束(基带单元/BBU) %d 已接满250个UE\n", BBU_index);
    }
    else
    {

        // 更新数组wave_ueser_mapPtrArray
        int index;
        index = 0;
        for (index; index < PER_WAVE_MAX_NUM; index++)
        {
            if (wave_ueser_mapPtrArray[wave_locall_index][index].State == 0)
            {
                CCH_resource_allocation(ue_bbu_index, wave_locall_index, get_ue_addr, get_ue_ip, get_wave_num, CCH_RB_FREQ_POINT_ARRAY_SIZE, get_ue_rnti);

                wave_ueser_mapPtrArray[wave_locall_index][index].State = 1;
                wave_ueser_mapPtrArray[wave_locall_index][index].U_RNTI = get_ue_rnti;
                // 下边两个值应该由用户的REQ传上来，现在先随便给个值
                wave_ueser_mapPtrArray[wave_locall_index][index].UE_Level = 2;
                wave_ueser_mapPtrArray[wave_locall_index][index].MCS = 5;
                wave_ueser_mapPtrArray[wave_locall_index][index].User_DataQueue_ptr = DL_data_buffer_Cvector_List[ue_bbu_index];
                per_wave_usernum[wave_locall_index]++;
                LOG("\n\nwave_locall_index = %d  UE_Level = %d  per_wave_usernum = %d\n\n", wave_locall_index, wave_ueser_mapPtrArray[wave_locall_index][index].UE_Level, per_wave_usernum[wave_locall_index]);
                LOG("size = %d\n", wave_ueser_mapPtrArray[wave_locall_index][index].User_DataQueue_ptr->size);
                LOG("User_DataQueue_ptr = %x\n", wave_ueser_mapPtrArray[wave_locall_index][index].User_DataQueue_ptr);
                LOG("wave_locall_index = %d   index = %d\n", wave_locall_index, index);
                break;
            }
        }

        // 为当前虚波位上的用户按照用户优先级排序
        sortLevel(wave_ueser_mapPtrArray, wave_locall_index);
        /******************/

        u8 RARSP_buff[128] = {0};
        type_RARSP_s *RARSP_frame = (type_RARSP_s *)RARSP_buff;

        RARSP_frame->CID = TYPE_RARSP;
        // RARSP_frame->SatMAC         =   own_address;
        memcpy(RARSP_frame->SatMAC.value, own_address.value, MY_MAC_ADDR_BYTE_NUM);
        // RARSP_frame->UEMAC          =   get_ue_addr;
        memcpy(RARSP_frame->UEMAC.value, get_ue_addr.value, MY_MAC_ADDR_BYTE_NUM);
        RARSP_frame->URNTI = get_ue_rnti; //msgb
        RARSP_frame->Uplink_num = 0;
        RARSP_frame->Uplink_fre = UserInfo_PtrArray[ue_bbu_index]->CCH_FrequencyPoint.slot0_uplink_FrequencyPoint;
        RARSP_frame->Uplink_mode = 0;
        RARSP_frame->Downlink_num = 0;
        RARSP_frame->Downlink_fre = UserInfo_PtrArray[ue_bbu_index]->CCH_FrequencyPoint.slot0_downlink_FrequencyPoint;
        RARSP_frame->Downlink_mode = 0;
        RARSP_frame->IP = RAMSG_frame->IP;
        u8 package_buff[256] = {0};
        u16 package_bufflen = 0;
        package_bufflen = Package_Frame(TYPE_RARSP, RARSP_buff, sizeof(type_RARSP_s), package_buff, sizeof(package_buff)); // 加帧头和帧尾
        UDP_write_rrc(package_buff, package_bufflen);
        LOG("Sat_mac_u send RARSP to Sat_rrc");

        access_ue_count++;

        // if(online_detect_start_en == 1)
        // {
        //     //起一个在线检测线程，判断用户是否在线
        //     //创建来自卫星的Package接收子线程

        //     online_detect_start_en = 0;

        //     pthread_t UOSD_thread_tid;
        //     pthread_create(&UOSD_thread_tid, NULL, UOSD_thread, NULL);
        // }else if(online_detect_start_en == 2)
        // {
        //     last_detect_time = 0;       //重置上一次用户在线状态检测的时间

        //     //线程唤醒
        //     pthread_mutex_lock(&m);
        //     pthread_cond_signal(&c);
        //     pthread_mutex_unlock(&m);

        //     online_detect_start_en = 0;
        // }
    }
}
void UE_OnlineReport_process(msg_all *msg)
{
    type_UE_OnlineReport_s *UE_OnlineReport_frame = (type_UE_OnlineReport_s *)msg->HEAD->DATA;

    if (UE_OnlineReport_frame->RRC_State == 1)
    {
        // 进入本代码区说明该UE的RRC连接正常，开始登记时间戳
        LOG("OnlineReport_process work normal");
        get_ue_rnti = UE_OnlineReport_frame->URNTI;                                     // 获取UE的RNTI
        u16 ue_index8_inBBU = (get_ue_rnti & 0xFFFF) % BBU_UE_MAXNUM;                   // 计算UE在波束（基带单元/BBU）内的索引号
        Online_Time_Table[ue_index8_inBBU] = getCurrentSecond() + ONLINE_DETECT_PERIOD; // 登记该UE的时间戳 = 当前OnlineReport接收时刻 + 1个检测周期；
        LOG("UE Time flag write:%d", Online_Time_Table[ue_index8_inBBU]);
    }
    else
    {
        LOG("OnlineReport_process does not work normal");
        // 进入本代码区说明该UE的RRC连接异常
        // 可在此添加功能拓展性代码以提高系统的健全性
    }
}
void UE_DataSum_process(msg_all *msg) // 处理收到的用户信息
{
    LOG("卫星MACU 收到UE_DATA_SUM");

    type_data_sum_s *UE_DataSum_frame = (type_data_sum_s *)msg->HEAD->DATA;

    u48 get_sat_id;
    memcpy(get_sat_id.value, UE_DataSum_frame->SatMAC.value, MY_MAC_ADDR_BYTE_NUM);

    if (strcmp(get_sat_id.value, own_address.value) == 0) // 说明是本地的消息，入队
    {
        // Push is datasum package
        App_queue_push(&Queue_UL_DataSum, (char *)msg->HEAD, getMacFrameLen(msg));
    }
}
void Sat_DataSum_process(msg_all *msg) // 卫星的数据放进下行数据队列
{
    LOG("卫星的数据传给ue");
    type_data_sum_s *Sat_DataSum_frame = (type_data_sum_s *)msg->HEAD->DATA;

    u48 get_sat_id;
    memcpy(get_sat_id.value, Sat_DataSum_frame->SatMAC.value, MY_MAC_ADDR_BYTE_NUM);
    if (strcmp(get_sat_id.value, own_address.value) == 0)
    {
        // Push is datasum package
        App_queue_push(&Queue_DL_DataSum, (char *)msg->HEAD, getMacFrameLen(msg));
    }
}
// 映射UE的RNTI到本基带处理单元的用户索引号
u16 RNTI_to_LocalIndex_map(u32 rnti, u16 bbu_index)
{
    u16 ue_global_index; // 用户在星内的全局索引号，等于RNTI低两字节
    u16 ue_local_index;  // 用户在本基带的索引号
    ue_global_index = rnti & 0x0000FFFF;
    ue_local_index = (u16)(ue_global_index - bbu_index * BBU_UE_MAXNUM);
    return ue_local_index;
}
u32 DIV_HEAD_Handle(type_div_head_s *div_head)
{
    if (package_totalNum_Array[div_head->URNTI] == 0)
    {
        package_totalNum_Array[div_head->URNTI] = div_head->Total_Num; // 总共有多少包数据传输
        pckage_recvNum_Array[div_head->URNTI] = 1;
        pckage_datalen_Array[div_head->URNTI] += (DIV_DATA_LEN - sizeof(type_div_head_s));
        App_queue_push(UE_data_recv_Queue_List[div_head->URNTI], div_head->Data, DIV_DATA_LEN - sizeof(type_div_head_s));
        return OK;
    }
    else
    {
        package_totalNum_Array[div_head->URNTI] = 0;
        pckage_recvNum_Array[div_head->URNTI] = 0;
        pckage_datalen_Array[div_head->URNTI] = 0;
        App_queue_init(UE_data_recv_Queue_List[div_head->URNTI]);
        LOG("div queue has lost package");
        return -1;
    }
}

u32 DIV_MID_Handle(type_div_mid_s *div_mid)
{
    u16 ue_local_index = RNTI_to_LocalIndex_map(div_mid->URNTI, BBU_index);
    if (package_totalNum_Array[ue_local_index] == 0)
    {
        LOG("div no head");
        return -1;
    }

    if (div_mid->T_Num == pckage_recvNum_Array[ue_local_index] + 1)
    {
        pckage_recvNum_Array[ue_local_index]++;
        pckage_datalen_Array[ue_local_index] += (DIV_DATA_LEN - sizeof(type_div_mid_s));
        App_queue_push(UE_data_recv_Queue_List[ue_local_index], div_mid->Data, DIV_DATA_LEN - sizeof(type_div_mid_s));
        return OK;
    }
    else
    {
        package_totalNum_Array[ue_local_index] = 0;
        pckage_recvNum_Array[ue_local_index] = 0;
        pckage_datalen_Array[ue_local_index] = 0;
        App_queue_init(UE_data_recv_Queue_List[ue_local_index]);
        return -1;
    }
}

u32 DIV_END_Handle(type_div_end_s *div_end)
{
    u16 ue_local_index = RNTI_to_LocalIndex_map(div_end->URNTI, BBU_index);
    if (package_totalNum_Array[ue_local_index] == 0)
    {
        LOG("div no head");
        return -1;
    }

    if (div_end->T_Num == pckage_recvNum_Array[ue_local_index] + 1)
    {
        pckage_recvNum_Array[ue_local_index]++;
        pckage_datalen_Array[ue_local_index] += (DIV_DATA_LEN - sizeof(type_div_end_s));
        App_queue_push(UE_data_recv_Queue_List[ue_local_index], div_end->Data, DIV_DATA_LEN - sizeof(type_div_end_s));

        u8 total_data[512] = {0};
        App_queue_pop(UE_data_recv_Queue_List[ue_local_index], total_data, pckage_datalen_Array[ue_local_index], 0x03);

        u8 package[600] = {0};
        u16 package_len = Package_Frame(TYPE_MSG_DIVISION, total_data, pckage_datalen_Array[ue_local_index], package, sizeof(package));
        UDP_write_rlc(package, package_len);

        package_totalNum_Array[ue_local_index] = 0;
        pckage_recvNum_Array[ue_local_index] = 0;
        pckage_datalen_Array[ue_local_index] = 0;

        return OK;
    }
}
/*非驻留类业务资源申请*/
void non_resident_resource_apply(type_non_resident_data_sum_s *UE_non_resident_DataSum_frame)
{
    LOG("卫星MACU 收到用户的非常驻业务资源申请");

    u32 get_rnti = UE_non_resident_DataSum_frame->URNTI;
    u16 ue_local_index = RNTI_to_LocalIndex_map(get_rnti, BBU_index);
    u32 get_data_length = UE_non_resident_DataSum_frame->data_length;
    u32 MCS = UserInfo_PtrArray[ue_local_index]->UL_MCS;
    u16 wave_num = UserInfo_PtrArray[ue_local_index]->BeamPositionIndex;
    u16 wave_local_index = WAVE_NUM_to_LocalIndex_map(wave_num);

    if (get_data_length > (PER_UE_MAX_DL_VOLUME - MAC_FRAME_LENGTH))
    {
        get_data_length = PER_UE_MAX_DL_VOLUME - MAC_FRAME_LENGTH;
    }

    // 计算需要给发起非驻留业务资源申请的用户分配的资源，向上取整
    u16 per_RB_ByteNum = cal_RB_carry_capacity(MCS);
    u16 RB_TotalNum = ceil((double)get_data_length / per_RB_ByteNum);

    u16 RB_begin;
    RB_begin = 0;
    if (UL_min_idleRB + RB_TotalNum <= SCH_RB_NUM)
    {
        // 计算分配给该用户那些RB
        RB_begin = UL_min_idleRB;
        // 记录给用户分配的下行业务资源信息
        UserInfo_PtrArray[ue_local_index]->UL_SCH_RB_begin = RB_begin;
        UserInfo_PtrArray[ue_local_index]->UL_SCH_RB_Total_NUM = RB_TotalNum;

        // 更新可用的上行非驻留业务的可用RB最小值；
        UL_min_idleRB += RB_TotalNum;
    }
    else
    {
        RB_TotalNum = SCH_RB_NUM - UL_min_idleRB;
        // 计算分配给该用户那些RB
        RB_begin = UL_min_idleRB;
        // 记录给用户分配的下行业务资源信息
        UserInfo_PtrArray[ue_local_index]->UL_SCH_RB_begin = RB_begin;
        UserInfo_PtrArray[ue_local_index]->UL_SCH_RB_Total_NUM = RB_TotalNum;

        // 更新可用的上行非驻留业务的可用RB最小值；
        UL_min_idleRB += RB_TotalNum;
    }
    LOG("RB_begin = %d    RB_TotalNum = %d \n", RB_begin, RB_TotalNum);
}

/*
映射UE的WAVE_NUM到本基带处理单元的wave_index
*/
u16 WAVE_NUM_to_LocalIndex_map(u16 wave_num)
{
    u16 wave_local_index;

    switch (BBU_index)
    {
    case 0:
        wave_local_index = wave_num - 0 * WAVE_NUM;
        break;
    case 1:
        wave_local_index = wave_num - 1 * WAVE_NUM;
        break;
    case 2:
        wave_local_index = wave_num - 2 * WAVE_NUM;
        break;
    case 3:
        wave_local_index = wave_num - 3 * WAVE_NUM;
        break;
    case 4:
        wave_local_index = wave_num - 2 * WAVE_NUM;
        break;
    case 5:
        wave_local_index = wave_num - 3 * WAVE_NUM;
        break;
    case 6:
        wave_local_index = wave_num - 0 * WAVE_NUM;
        break;
    case 7:
        wave_local_index = wave_num - 1 * WAVE_NUM;
        break;
    default:
        break;
    }

    return wave_local_index;
}

/*
皇甫阳阳
由基带板wave_index映射到UE的WAVE_NUM
*/
u16 wave_local_index_to_WAVE_NUM(u16 wave_locall_index)
{
    u16 wave_num;

    switch (BBU_index)
    {
    case 0:
        wave_num = wave_locall_index + 0 * WAVE_NUM;
        break;
    case 1:
        wave_num = wave_locall_index + 1 * WAVE_NUM;
        break;
    case 2:
        wave_num = wave_locall_index + 2 * WAVE_NUM;
        break;
    case 3:
        wave_num = wave_locall_index + 3 * WAVE_NUM;
        break;
    case 4:
        wave_num = wave_locall_index + 2 * WAVE_NUM;
        break;
    case 5:
        wave_num = wave_locall_index + 3 * WAVE_NUM;
        break;
    case 6:
        wave_num = wave_locall_index + 0 * WAVE_NUM;
        break;
    case 7:
        wave_num = wave_locall_index + 1 * WAVE_NUM;
        break;
    default:
        break;
    }

    return wave_num;
}
// 控制信道资源分配函数，分配固定数量的CCH_RB,从rrc中取出信息放在控制信道中
void CCH_resource_allocation(u16 ue_local_index, u16 wave_locall_index, u48 ue_addr, u32 ip, u16 wave_num, u8 CCH_RB_num, u32 rnti)
{
    u8 tmp_ul_rb_id_buff[8] = {0};
    u8 tmp_dl_rb_id_buff[8] = {0};

    u16 *tmp_ul_rb_id = NULL;
    u16 *tmp_dl_rb_id = NULL;

    u8 index;
    index = 0;
    for (index; index < CCH_RB_num; index++)
    {
        if (index == 0)
        {
            // CCH资源获取
            /*皇甫阳阳*/
            App_queue_pop(UL_Idle_CCH_RB_Queue_List[wave_locall_index], tmp_ul_rb_id_buff, sizeof(u16 *), 0x03);
            App_queue_pop(DL_Idle_CCH_RB_Queue_List[wave_locall_index], tmp_dl_rb_id_buff, sizeof(u16 *), 0x03);

            tmp_ul_rb_id = (u16 *)tmp_ul_rb_id_buff;
            tmp_dl_rb_id = (u16 *)tmp_dl_rb_id_buff;

            u16 wave_user_map_index = per_wave_usernum[wave_locall_index]; // 意思是在第一次把用户的信息取上

            UserInfo_PtrArray[ue_local_index]->state = 1;
            UserInfo_PtrArray[ue_local_index]->U_RNTI = rnti;
            memcpy(UserInfo_PtrArray[ue_local_index]->UE_MAC_addr.value, ue_addr.value, MY_MAC_ADDR_BYTE_NUM);
            // UserInfo_PtrArray[ue_local_index]->UE_MAC_addr                                      = ue_addr;
            UserInfo_PtrArray[ue_local_index]->BeamPositionIndex = wave_num;
            UserInfo_PtrArray[ue_local_index]->wave_user_index = wave_user_map_index;
            UserInfo_PtrArray[ue_local_index]->IPv4 = ip;
            UserInfo_PtrArray[ue_local_index]->CCH_SlotIndex.slot0_uplink_en = 1;
            UserInfo_PtrArray[ue_local_index]->CCH_SlotIndex.slot0_downlink_en = 1;
            UserInfo_PtrArray[ue_local_index]->CCH_FrequencyPoint.slot0_uplink_FrequencyPoint = *tmp_ul_rb_id; // CCH只有一个时隙，因此RB_ID与时隙中RB的序号值相同
            UserInfo_PtrArray[ue_local_index]->CCH_FrequencyPoint.slot0_downlink_FrequencyPoint = *tmp_dl_rb_id;
        }
        else
        {
            // 仅需记录最开始的RB序号，后续的从列表弹出即可
            App_queue_pop(UL_Idle_CCH_RB_Queue_List[wave_locall_index], tmp_ul_rb_id_buff, sizeof(u16 *), 0x02);
            App_queue_pop(DL_Idle_CCH_RB_Queue_List[wave_locall_index], tmp_dl_rb_id_buff, sizeof(u16 *), 0x02);
        }
    }
}

// CCH资源释放函数, 该函数仅将控制信道CCH_RB重新插入空闲CCH_RB链表
// 因为CCH资源释放表示用户断开连接或者需要切换，因此UserInfo_PtrArray的内容由相应的初始化函数完成初始化
void CCH_resource_release(u16 bbu_ue_index, u16 wave_local_index, u8 CCH_RB_num)
{
    u8 index;

    u16 ULCCH_RB_begin = UserInfo_PtrArray[bbu_ue_index]->CCH_FrequencyPoint.slot0_uplink_FrequencyPoint;
    u16 DLCCH_RB_begin = UserInfo_PtrArray[bbu_ue_index]->CCH_FrequencyPoint.slot0_downlink_FrequencyPoint;

    index = 0;
    for (index; index < CCH_RB_num; index++)
    {
        App_queue_push(UL_Idle_CCH_RB_Queue_List[wave_local_index], (char *)&cch_rb_id[ULCCH_RB_begin + index], sizeof(u16 *));
        App_queue_push(UL_Idle_CCH_RB_Queue_List[wave_local_index], (char *)&cch_rb_id[DLCCH_RB_begin + index], sizeof(u16 *));
    }
}
// 冒泡排序
void sortLevel(UserInfo_Block UEinWaveTable[WAVE_NUM][PER_WAVE_MAX_NUM], u16 wave_locall_index)
{
    int UEindex;
    UserInfo_Block p;
    for (UEindex = 0; UEindex < PER_WAVE_MAX_NUM; UEindex++)
    {
        int i;
        for (i = 0; i < PER_WAVE_MAX_NUM - UEindex - 1; i++)
        {
            if (UEinWaveTable[wave_locall_index][i + 1].State == 0)
            {
                break;
            }
            if (UEinWaveTable[wave_locall_index][i].UE_Level > UEinWaveTable[wave_locall_index][i + 1].UE_Level)
            {
                // 定义一个结构体P
                p.U_RNTI = UEinWaveTable[wave_locall_index][i].U_RNTI;
                p.UE_Level = UEinWaveTable[wave_locall_index][i].UE_Level;
                p.User_DataQueue_ptr = UEinWaveTable[wave_locall_index][i].User_DataQueue_ptr;
                p.MCS = UEinWaveTable[wave_locall_index][i].MCS;

                UEinWaveTable[wave_locall_index][i].U_RNTI = UEinWaveTable[wave_locall_index][i + 1].U_RNTI;
                UEinWaveTable[wave_locall_index][i].UE_Level = UEinWaveTable[wave_locall_index][i + 1].UE_Level;
                UEinWaveTable[wave_locall_index][i].User_DataQueue_ptr = UEinWaveTable[wave_locall_index][i + 1].User_DataQueue_ptr;
                UEinWaveTable[wave_locall_index][i].MCS = UEinWaveTable[wave_locall_index][i + 1].MCS;

                UEinWaveTable[wave_locall_index][i + 1].U_RNTI = p.U_RNTI;
                UEinWaveTable[wave_locall_index][i + 1].UE_Level = p.UE_Level;
                UEinWaveTable[wave_locall_index][i + 1].User_DataQueue_ptr = p.User_DataQueue_ptr;
                UEinWaveTable[wave_locall_index][i + 1].MCS = p.MCS;
            }
        }
    }
}

//***************** MAC PROCESS STATE FROM OPNET PROJECT V6.F *****************//
void MAC_U_init()
{
    // 获取本星的MAC地址
    get_own_address(own_address.value);
    UserInfo_PtrArray_init(UserInfo_PtrArray, BBU_UE_MAXNUM);
    online_time_table_init(Online_Time_Table);
    init_RB_ID(cch_rb_id, sch_rb_id);                                                      // 初始化跳波束id
    init_CCH_idlerb_list(UL_Idle_CCH_RB_Queue_List, DL_Idle_CCH_RB_Queue_List, cch_rb_id); // 控制信道空闲资源列表
    App_queue_init(&Queue_UL_DataSum);                                                     // 上行链路数据
    App_queue_init(&Queue_DL_DataSum);                                                     // 下行链路数据
    Array_init(package_totalNum_Array, BBU_UE_MAXNUM, 0);
    Array_init(pckage_recvNum_Array, BBU_UE_MAXNUM, 0);
    Array_init(pckage_datalen_Array, BBU_UE_MAXNUM, 0);
    wave_ueser_mapPtrArray_init(wave_ueser_mapPtrArray, WAVE_NUM); // 下行业务缓存与用户队列注册
    per_wave_usernum_init(per_wave_usernum, WAVE_NUM);             // 每个虚波位接入多少用户的数组
    Frame_init(&frame_t);
    init_DL_data_buffer_Cvector_List();
    init_Mac_to_PHY_data();
    init_RB_Reception_Info();
    App_queue_init(&Queue_DL_CCH_DATA_Packaging_Buffer);
    App_queue_init(&Queue_DL_SCH_DATA_Packaging_Buffer);
}
void Frame_init(frame_info **frame_p)
{
    // 检查传入指针是否为 NULL（指针的地址必须合法）
    if (frame_p == NULL || *frame_p == NULL)
    {
        *frame_p = (frame_info *)malloc(sizeof(frame_info));
        if (*frame_p == NULL)
        { // 检查 malloc 是否成功
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
    }

    // 现在可以安全地访问指针所指向的结构体
    (*frame_p)->frame_num = 0; // 初始化参数
    (*frame_p)->slot_num = 0;
    (*frame_p)->sub_frame_num = 0;
}
// 数组统一赋初值
void Array_init(u16 arr[], u32 size, u16 value)
{
    u32 ArrIndex = 0;
    for (ArrIndex; ArrIndex < size; ArrIndex++)
    {
        arr[ArrIndex] = value;
    }
}
// 用户注册信息_指针数组_初始化
void UserInfo_PtrArray_init(UserInfo **PtrArray, u32 size)
{
    u32 ArrayMemberIndex = 0;
    for (ArrayMemberIndex; ArrayMemberIndex < size; ArrayMemberIndex++)
    {
        UserInfo *newUser = (UserInfo *)malloc(sizeof(UserInfo));
        UserInfo_init(newUser);

        PtrArray[ArrayMemberIndex] = newUser;
    }
}

// 用户注册信息初始化
void UserInfo_init(UserInfo *user)
{
    user->state = 0;
    user->U_RNTI = 0xFFFFFFFF;
    memset(user->UE_MAC_addr.value, 0, MY_MAC_ADDR_BYTE_NUM);
    user->BeamIndex = 0xFF;
    user->BeamPositionIndex = 255;

    /*皇甫阳阳*/
    user->wave_user_index = 0xFFFF;
    user->UL_MCS = 0xFFFFFFFF;
    user->DL_MCS = 0xFFFFFFFF;
    user->IPv4 = 0xFFFFFFFF;
    user->UL_SCH_RB_begin = 0xFFFF;
    user->UL_SCH_RB_Total_NUM = 0xFFFF;
    user->eatrly_UL_SCH_RB_begin = 0xFFFF;
    user->eatrly_UL_SCH_RB_Total_NUM = 0xFFFF;
    user->DL_SCH_RB_begin = 0xFFFF;
    user->DL_SCH_RB_Total_NUM = 0xFFFF;

    user->CCH_SlotIndex.slot0_uplink_en = 0;
    user->CCH_SlotIndex.slot0_downlink_en = 0;
    user->CCH_FrequencyPoint.slot0_uplink_FrequencyPoint = 255;
    user->CCH_FrequencyPoint.slot0_downlink_FrequencyPoint = 255;
    user->SCH_SlotIndex.slot1_uplink_en = 0;
    user->SCH_SlotIndex.slot2_uplink_en = 0;
    user->SCH_SlotIndex.slot3_uplink_en = 0;
    user->SCH_SlotIndex.slot1_downlink_en = 0;
    user->SCH_SlotIndex.slot2_downlink_en = 0;
    user->SCH_SlotIndex.slot3_downlink_en = 0;
    Array_init(user->SCH_FrequencyPoint.slot1_uplink_FrequencyPoint, SCH_RB_FREQ_POINT_ARRAY_SIZE, 255);
    memcpy(user->SCH_FrequencyPoint.slot1_downlink_FrequencyPoint, user->SCH_FrequencyPoint.slot1_uplink_FrequencyPoint, sizeof(user->SCH_FrequencyPoint.slot1_uplink_FrequencyPoint));
    memcpy(user->SCH_FrequencyPoint.slot2_uplink_FrequencyPoint, user->SCH_FrequencyPoint.slot1_uplink_FrequencyPoint, sizeof(user->SCH_FrequencyPoint.slot1_uplink_FrequencyPoint));
    memcpy(user->SCH_FrequencyPoint.slot2_downlink_FrequencyPoint, user->SCH_FrequencyPoint.slot1_uplink_FrequencyPoint, sizeof(user->SCH_FrequencyPoint.slot1_uplink_FrequencyPoint));
    memcpy(user->SCH_FrequencyPoint.slot3_uplink_FrequencyPoint, user->SCH_FrequencyPoint.slot1_uplink_FrequencyPoint, sizeof(user->SCH_FrequencyPoint.slot1_uplink_FrequencyPoint));
    memcpy(user->SCH_FrequencyPoint.slot3_downlink_FrequencyPoint, user->SCH_FrequencyPoint.slot1_uplink_FrequencyPoint, sizeof(user->SCH_FrequencyPoint.slot1_uplink_FrequencyPoint));
    user->SCH_FrequencyPoint_AllotNum.slot1_uplink_FrequencyPoint_AllotNum = 0;
    user->SCH_FrequencyPoint_AllotNum.slot2_uplink_FrequencyPoint_AllotNum = 0;
    user->SCH_FrequencyPoint_AllotNum.slot3_uplink_FrequencyPoint_AllotNum = 0;
    user->SCH_FrequencyPoint_AllotNum.slot1_downlink_FrequencyPoint_AllotNum = 0;
    user->SCH_FrequencyPoint_AllotNum.slot2_downlink_FrequencyPoint_AllotNum = 0;
    user->SCH_FrequencyPoint_AllotNum.slot3_downlink_FrequencyPoint_AllotNum = 0;
}

// 初始化状态报告时间
void online_time_table_init(int *online_time_table)
{
    u16 line_index = 0;

    for (line_index; line_index < BBU_UE_MAXNUM; line_index++)
    {
        online_time_table[line_index] = -1; //-1表示当前无用户切入
    }
}
// 初始化跳波束RBID
void init_RB_ID(u16 *cch_rbid, u16 sch_rbid[][RB_TOTALNUM])
{
    u16 cch_rb_index = 0;
    u16 sch_slt_index = 0;
    u16 sch_rb_index = 0;

    for (cch_rb_index; cch_rb_index < RB_TOTALNUM; cch_rb_index++)
    {
        cch_rbid[cch_rb_index] = cch_rb_index;
        // printf("%hu ", cch_rb_id[cch_rb_index]);
    }

    for (sch_slt_index; sch_slt_index < SSLOT_TOTALNUM; sch_slt_index++)
    {
        for (sch_rb_index; sch_rb_index < RB_TOTALNUM; sch_rb_index++)
        {
            sch_rbid[sch_slt_index][sch_rb_index] = sch_slt_index * RB_TOTALNUM + sch_rb_index;
            // printf("%hu ", sch_rbid[sch_slt_index][sch_rb_index]);
        }
        sch_rb_index = 0;
    }
}
// 创建跳波束控制信道空闲资源列表并完成初始化
void init_CCH_idlerb_list(App_queue **ulcch_idlerb_list, App_queue **dlcch_idlerb_list, u16 *cch_rbid)
{
    u16 Wave_index = 0;
    u16 rb_index = 0;

    ulcch_idlerb_list[0] = &Queue_ULCCH_idleRB_fifo_000;
    ulcch_idlerb_list[1] = &Queue_ULCCH_idleRB_fifo_001;
    ulcch_idlerb_list[2] = &Queue_ULCCH_idleRB_fifo_002;
    ulcch_idlerb_list[3] = &Queue_ULCCH_idleRB_fifo_003;
    ulcch_idlerb_list[4] = &Queue_ULCCH_idleRB_fifo_004;
    ulcch_idlerb_list[5] = &Queue_ULCCH_idleRB_fifo_005;
    ulcch_idlerb_list[6] = &Queue_ULCCH_idleRB_fifo_006;
    ulcch_idlerb_list[7] = &Queue_ULCCH_idleRB_fifo_007;
    ulcch_idlerb_list[8] = &Queue_ULCCH_idleRB_fifo_008;
    ulcch_idlerb_list[9] = &Queue_ULCCH_idleRB_fifo_009;
    ulcch_idlerb_list[10] = &Queue_ULCCH_idleRB_fifo_010;
    ulcch_idlerb_list[11] = &Queue_ULCCH_idleRB_fifo_011;
    ulcch_idlerb_list[12] = &Queue_ULCCH_idleRB_fifo_012;
    ulcch_idlerb_list[13] = &Queue_ULCCH_idleRB_fifo_013;
    ulcch_idlerb_list[14] = &Queue_ULCCH_idleRB_fifo_014;
    ulcch_idlerb_list[15] = &Queue_ULCCH_idleRB_fifo_015;
    ulcch_idlerb_list[16] = &Queue_ULCCH_idleRB_fifo_016;
    ulcch_idlerb_list[17] = &Queue_ULCCH_idleRB_fifo_017;
    ulcch_idlerb_list[18] = &Queue_ULCCH_idleRB_fifo_018;
    ulcch_idlerb_list[19] = &Queue_ULCCH_idleRB_fifo_019;
    ulcch_idlerb_list[20] = &Queue_ULCCH_idleRB_fifo_020;
    ulcch_idlerb_list[21] = &Queue_ULCCH_idleRB_fifo_021;
    ulcch_idlerb_list[22] = &Queue_ULCCH_idleRB_fifo_022;
    ulcch_idlerb_list[23] = &Queue_ULCCH_idleRB_fifo_023;
    ulcch_idlerb_list[24] = &Queue_ULCCH_idleRB_fifo_024;
    ulcch_idlerb_list[25] = &Queue_ULCCH_idleRB_fifo_025;
    ulcch_idlerb_list[26] = &Queue_ULCCH_idleRB_fifo_026;
    ulcch_idlerb_list[27] = &Queue_ULCCH_idleRB_fifo_027;
    ulcch_idlerb_list[28] = &Queue_ULCCH_idleRB_fifo_028;
    ulcch_idlerb_list[29] = &Queue_ULCCH_idleRB_fifo_029;
    ulcch_idlerb_list[30] = &Queue_ULCCH_idleRB_fifo_030;
    ulcch_idlerb_list[31] = &Queue_ULCCH_idleRB_fifo_031;
    ulcch_idlerb_list[32] = &Queue_ULCCH_idleRB_fifo_032;
    ulcch_idlerb_list[33] = &Queue_ULCCH_idleRB_fifo_033;
    ulcch_idlerb_list[34] = &Queue_ULCCH_idleRB_fifo_034;
    ulcch_idlerb_list[35] = &Queue_ULCCH_idleRB_fifo_035;
    ulcch_idlerb_list[36] = &Queue_ULCCH_idleRB_fifo_036;
    ulcch_idlerb_list[37] = &Queue_ULCCH_idleRB_fifo_037;
    ulcch_idlerb_list[38] = &Queue_ULCCH_idleRB_fifo_038;
    ulcch_idlerb_list[39] = &Queue_ULCCH_idleRB_fifo_039;
    ulcch_idlerb_list[40] = &Queue_ULCCH_idleRB_fifo_040;
    ulcch_idlerb_list[41] = &Queue_ULCCH_idleRB_fifo_041;
    ulcch_idlerb_list[42] = &Queue_ULCCH_idleRB_fifo_042;
    ulcch_idlerb_list[43] = &Queue_ULCCH_idleRB_fifo_043;
    ulcch_idlerb_list[44] = &Queue_ULCCH_idleRB_fifo_044;
    ulcch_idlerb_list[45] = &Queue_ULCCH_idleRB_fifo_045;
    ulcch_idlerb_list[46] = &Queue_ULCCH_idleRB_fifo_046;
    ulcch_idlerb_list[47] = &Queue_ULCCH_idleRB_fifo_047;
    ulcch_idlerb_list[48] = &Queue_ULCCH_idleRB_fifo_048;
    ulcch_idlerb_list[49] = &Queue_ULCCH_idleRB_fifo_049;
    ulcch_idlerb_list[50] = &Queue_ULCCH_idleRB_fifo_050;
    ulcch_idlerb_list[51] = &Queue_ULCCH_idleRB_fifo_051;
    ulcch_idlerb_list[52] = &Queue_ULCCH_idleRB_fifo_052;
    ulcch_idlerb_list[53] = &Queue_ULCCH_idleRB_fifo_053;
    ulcch_idlerb_list[54] = &Queue_ULCCH_idleRB_fifo_054;
    ulcch_idlerb_list[55] = &Queue_ULCCH_idleRB_fifo_055;
    ulcch_idlerb_list[56] = &Queue_ULCCH_idleRB_fifo_056;
    ulcch_idlerb_list[57] = &Queue_ULCCH_idleRB_fifo_057;
    ulcch_idlerb_list[58] = &Queue_ULCCH_idleRB_fifo_058;
    ulcch_idlerb_list[59] = &Queue_ULCCH_idleRB_fifo_059;
    ulcch_idlerb_list[60] = &Queue_ULCCH_idleRB_fifo_060;
    ulcch_idlerb_list[61] = &Queue_ULCCH_idleRB_fifo_061;
    ulcch_idlerb_list[62] = &Queue_ULCCH_idleRB_fifo_062;
    ulcch_idlerb_list[63] = &Queue_ULCCH_idleRB_fifo_063;
    ulcch_idlerb_list[64] = &Queue_ULCCH_idleRB_fifo_064;
    ulcch_idlerb_list[65] = &Queue_ULCCH_idleRB_fifo_065;
    ulcch_idlerb_list[66] = &Queue_ULCCH_idleRB_fifo_066;
    ulcch_idlerb_list[67] = &Queue_ULCCH_idleRB_fifo_067;
    ulcch_idlerb_list[68] = &Queue_ULCCH_idleRB_fifo_068;
    ulcch_idlerb_list[69] = &Queue_ULCCH_idleRB_fifo_069;
    ulcch_idlerb_list[70] = &Queue_ULCCH_idleRB_fifo_070;
    ulcch_idlerb_list[71] = &Queue_ULCCH_idleRB_fifo_071;
    ulcch_idlerb_list[72] = &Queue_ULCCH_idleRB_fifo_072;
    ulcch_idlerb_list[73] = &Queue_ULCCH_idleRB_fifo_073;
    ulcch_idlerb_list[74] = &Queue_ULCCH_idleRB_fifo_074;
    ulcch_idlerb_list[75] = &Queue_ULCCH_idleRB_fifo_075;
    ulcch_idlerb_list[76] = &Queue_ULCCH_idleRB_fifo_076;
    ulcch_idlerb_list[77] = &Queue_ULCCH_idleRB_fifo_077;
    ulcch_idlerb_list[78] = &Queue_ULCCH_idleRB_fifo_078;
    ulcch_idlerb_list[79] = &Queue_ULCCH_idleRB_fifo_079;
    ulcch_idlerb_list[80] = &Queue_ULCCH_idleRB_fifo_080;
    ulcch_idlerb_list[81] = &Queue_ULCCH_idleRB_fifo_081;
    ulcch_idlerb_list[82] = &Queue_ULCCH_idleRB_fifo_082;
    ulcch_idlerb_list[83] = &Queue_ULCCH_idleRB_fifo_083;
    ulcch_idlerb_list[84] = &Queue_ULCCH_idleRB_fifo_084;
    ulcch_idlerb_list[85] = &Queue_ULCCH_idleRB_fifo_085;
    ulcch_idlerb_list[86] = &Queue_ULCCH_idleRB_fifo_086;
    ulcch_idlerb_list[87] = &Queue_ULCCH_idleRB_fifo_087;
    ulcch_idlerb_list[88] = &Queue_ULCCH_idleRB_fifo_088;
    ulcch_idlerb_list[89] = &Queue_ULCCH_idleRB_fifo_089;
    ulcch_idlerb_list[90] = &Queue_ULCCH_idleRB_fifo_090;
    ulcch_idlerb_list[91] = &Queue_ULCCH_idleRB_fifo_091;
    ulcch_idlerb_list[92] = &Queue_ULCCH_idleRB_fifo_092;
    ulcch_idlerb_list[93] = &Queue_ULCCH_idleRB_fifo_093;
    ulcch_idlerb_list[94] = &Queue_ULCCH_idleRB_fifo_094;
    ulcch_idlerb_list[95] = &Queue_ULCCH_idleRB_fifo_095;
    ulcch_idlerb_list[96] = &Queue_ULCCH_idleRB_fifo_096;
    ulcch_idlerb_list[97] = &Queue_ULCCH_idleRB_fifo_097;
    ulcch_idlerb_list[98] = &Queue_ULCCH_idleRB_fifo_098;
    ulcch_idlerb_list[99] = &Queue_ULCCH_idleRB_fifo_099;
    ulcch_idlerb_list[100] = &Queue_ULCCH_idleRB_fifo_100;
    ulcch_idlerb_list[101] = &Queue_ULCCH_idleRB_fifo_101;
    ulcch_idlerb_list[102] = &Queue_ULCCH_idleRB_fifo_102;
    ulcch_idlerb_list[103] = &Queue_ULCCH_idleRB_fifo_103;
    ulcch_idlerb_list[104] = &Queue_ULCCH_idleRB_fifo_104;
    ulcch_idlerb_list[105] = &Queue_ULCCH_idleRB_fifo_105;
    ulcch_idlerb_list[106] = &Queue_ULCCH_idleRB_fifo_106;
    ulcch_idlerb_list[107] = &Queue_ULCCH_idleRB_fifo_107;
    ulcch_idlerb_list[108] = &Queue_ULCCH_idleRB_fifo_108;
    ulcch_idlerb_list[109] = &Queue_ULCCH_idleRB_fifo_109;
    ulcch_idlerb_list[110] = &Queue_ULCCH_idleRB_fifo_110;
    ulcch_idlerb_list[111] = &Queue_ULCCH_idleRB_fifo_111;
    ulcch_idlerb_list[112] = &Queue_ULCCH_idleRB_fifo_112;
    ulcch_idlerb_list[113] = &Queue_ULCCH_idleRB_fifo_113;
    ulcch_idlerb_list[114] = &Queue_ULCCH_idleRB_fifo_114;
    ulcch_idlerb_list[115] = &Queue_ULCCH_idleRB_fifo_115;
    ulcch_idlerb_list[116] = &Queue_ULCCH_idleRB_fifo_116;
    ulcch_idlerb_list[117] = &Queue_ULCCH_idleRB_fifo_117;
    ulcch_idlerb_list[118] = &Queue_ULCCH_idleRB_fifo_118;
    ulcch_idlerb_list[119] = &Queue_ULCCH_idleRB_fifo_119;
    ulcch_idlerb_list[120] = &Queue_ULCCH_idleRB_fifo_120;
    ulcch_idlerb_list[121] = &Queue_ULCCH_idleRB_fifo_121;
    ulcch_idlerb_list[122] = &Queue_ULCCH_idleRB_fifo_122;
    ulcch_idlerb_list[123] = &Queue_ULCCH_idleRB_fifo_123;
    ulcch_idlerb_list[124] = &Queue_ULCCH_idleRB_fifo_124;
    ulcch_idlerb_list[125] = &Queue_ULCCH_idleRB_fifo_125;
    ulcch_idlerb_list[126] = &Queue_ULCCH_idleRB_fifo_126;
    ulcch_idlerb_list[127] = &Queue_ULCCH_idleRB_fifo_127;
    ulcch_idlerb_list[128] = &Queue_ULCCH_idleRB_fifo_128;
    ulcch_idlerb_list[129] = &Queue_ULCCH_idleRB_fifo_129;
    ulcch_idlerb_list[130] = &Queue_ULCCH_idleRB_fifo_130;
    ulcch_idlerb_list[131] = &Queue_ULCCH_idleRB_fifo_131;
    ulcch_idlerb_list[132] = &Queue_ULCCH_idleRB_fifo_132;
    ulcch_idlerb_list[133] = &Queue_ULCCH_idleRB_fifo_133;
    ulcch_idlerb_list[134] = &Queue_ULCCH_idleRB_fifo_134;
    ulcch_idlerb_list[135] = &Queue_ULCCH_idleRB_fifo_135;
    ulcch_idlerb_list[136] = &Queue_ULCCH_idleRB_fifo_136;
    ulcch_idlerb_list[137] = &Queue_ULCCH_idleRB_fifo_137;
    ulcch_idlerb_list[138] = &Queue_ULCCH_idleRB_fifo_138;
    ulcch_idlerb_list[139] = &Queue_ULCCH_idleRB_fifo_139;
    ulcch_idlerb_list[140] = &Queue_ULCCH_idleRB_fifo_140;
    ulcch_idlerb_list[141] = &Queue_ULCCH_idleRB_fifo_141;
    ulcch_idlerb_list[142] = &Queue_ULCCH_idleRB_fifo_142;
    ulcch_idlerb_list[143] = &Queue_ULCCH_idleRB_fifo_143;
    ulcch_idlerb_list[144] = &Queue_ULCCH_idleRB_fifo_144;
    ulcch_idlerb_list[145] = &Queue_ULCCH_idleRB_fifo_145;
    ulcch_idlerb_list[146] = &Queue_ULCCH_idleRB_fifo_146;
    ulcch_idlerb_list[147] = &Queue_ULCCH_idleRB_fifo_147;
    ulcch_idlerb_list[148] = &Queue_ULCCH_idleRB_fifo_148;
    ulcch_idlerb_list[149] = &Queue_ULCCH_idleRB_fifo_149;

    dlcch_idlerb_list[0] = &Queue_DLCCH_idleRB_fifo_000;
    dlcch_idlerb_list[1] = &Queue_DLCCH_idleRB_fifo_001;
    dlcch_idlerb_list[2] = &Queue_DLCCH_idleRB_fifo_002;
    dlcch_idlerb_list[3] = &Queue_DLCCH_idleRB_fifo_003;
    dlcch_idlerb_list[4] = &Queue_DLCCH_idleRB_fifo_004;
    dlcch_idlerb_list[5] = &Queue_DLCCH_idleRB_fifo_005;
    dlcch_idlerb_list[6] = &Queue_DLCCH_idleRB_fifo_006;
    dlcch_idlerb_list[7] = &Queue_DLCCH_idleRB_fifo_007;
    dlcch_idlerb_list[8] = &Queue_DLCCH_idleRB_fifo_008;
    dlcch_idlerb_list[9] = &Queue_DLCCH_idleRB_fifo_009;
    dlcch_idlerb_list[10] = &Queue_DLCCH_idleRB_fifo_010;
    dlcch_idlerb_list[11] = &Queue_DLCCH_idleRB_fifo_011;
    dlcch_idlerb_list[12] = &Queue_DLCCH_idleRB_fifo_012;
    dlcch_idlerb_list[13] = &Queue_DLCCH_idleRB_fifo_013;
    dlcch_idlerb_list[14] = &Queue_DLCCH_idleRB_fifo_014;
    dlcch_idlerb_list[15] = &Queue_DLCCH_idleRB_fifo_015;
    dlcch_idlerb_list[16] = &Queue_DLCCH_idleRB_fifo_016;
    dlcch_idlerb_list[17] = &Queue_DLCCH_idleRB_fifo_017;
    dlcch_idlerb_list[18] = &Queue_DLCCH_idleRB_fifo_018;
    dlcch_idlerb_list[19] = &Queue_DLCCH_idleRB_fifo_019;
    dlcch_idlerb_list[20] = &Queue_DLCCH_idleRB_fifo_020;
    dlcch_idlerb_list[21] = &Queue_DLCCH_idleRB_fifo_021;
    dlcch_idlerb_list[22] = &Queue_DLCCH_idleRB_fifo_022;
    dlcch_idlerb_list[23] = &Queue_DLCCH_idleRB_fifo_023;
    dlcch_idlerb_list[24] = &Queue_DLCCH_idleRB_fifo_024;
    dlcch_idlerb_list[25] = &Queue_DLCCH_idleRB_fifo_025;
    dlcch_idlerb_list[26] = &Queue_DLCCH_idleRB_fifo_026;
    dlcch_idlerb_list[27] = &Queue_DLCCH_idleRB_fifo_027;
    dlcch_idlerb_list[28] = &Queue_DLCCH_idleRB_fifo_028;
    dlcch_idlerb_list[29] = &Queue_DLCCH_idleRB_fifo_029;
    dlcch_idlerb_list[30] = &Queue_DLCCH_idleRB_fifo_030;
    dlcch_idlerb_list[31] = &Queue_DLCCH_idleRB_fifo_031;
    dlcch_idlerb_list[32] = &Queue_DLCCH_idleRB_fifo_032;
    dlcch_idlerb_list[33] = &Queue_DLCCH_idleRB_fifo_033;
    dlcch_idlerb_list[34] = &Queue_DLCCH_idleRB_fifo_034;
    dlcch_idlerb_list[35] = &Queue_DLCCH_idleRB_fifo_035;
    dlcch_idlerb_list[36] = &Queue_DLCCH_idleRB_fifo_036;
    dlcch_idlerb_list[37] = &Queue_DLCCH_idleRB_fifo_037;
    dlcch_idlerb_list[38] = &Queue_DLCCH_idleRB_fifo_038;
    dlcch_idlerb_list[39] = &Queue_DLCCH_idleRB_fifo_039;
    dlcch_idlerb_list[40] = &Queue_DLCCH_idleRB_fifo_040;
    dlcch_idlerb_list[41] = &Queue_DLCCH_idleRB_fifo_041;
    dlcch_idlerb_list[42] = &Queue_DLCCH_idleRB_fifo_042;
    dlcch_idlerb_list[43] = &Queue_DLCCH_idleRB_fifo_043;
    dlcch_idlerb_list[44] = &Queue_DLCCH_idleRB_fifo_044;
    dlcch_idlerb_list[45] = &Queue_DLCCH_idleRB_fifo_045;
    dlcch_idlerb_list[46] = &Queue_DLCCH_idleRB_fifo_046;
    dlcch_idlerb_list[47] = &Queue_DLCCH_idleRB_fifo_047;
    dlcch_idlerb_list[48] = &Queue_DLCCH_idleRB_fifo_048;
    dlcch_idlerb_list[49] = &Queue_DLCCH_idleRB_fifo_049;
    dlcch_idlerb_list[50] = &Queue_DLCCH_idleRB_fifo_050;
    dlcch_idlerb_list[51] = &Queue_DLCCH_idleRB_fifo_051;
    dlcch_idlerb_list[52] = &Queue_DLCCH_idleRB_fifo_052;
    dlcch_idlerb_list[53] = &Queue_DLCCH_idleRB_fifo_053;
    dlcch_idlerb_list[54] = &Queue_DLCCH_idleRB_fifo_054;
    dlcch_idlerb_list[55] = &Queue_DLCCH_idleRB_fifo_055;
    dlcch_idlerb_list[56] = &Queue_DLCCH_idleRB_fifo_056;
    dlcch_idlerb_list[57] = &Queue_DLCCH_idleRB_fifo_057;
    dlcch_idlerb_list[58] = &Queue_DLCCH_idleRB_fifo_058;
    dlcch_idlerb_list[59] = &Queue_DLCCH_idleRB_fifo_059;
    dlcch_idlerb_list[60] = &Queue_DLCCH_idleRB_fifo_060;
    dlcch_idlerb_list[61] = &Queue_DLCCH_idleRB_fifo_061;
    dlcch_idlerb_list[62] = &Queue_DLCCH_idleRB_fifo_062;
    dlcch_idlerb_list[63] = &Queue_DLCCH_idleRB_fifo_063;
    dlcch_idlerb_list[64] = &Queue_DLCCH_idleRB_fifo_064;
    dlcch_idlerb_list[65] = &Queue_DLCCH_idleRB_fifo_065;
    dlcch_idlerb_list[66] = &Queue_DLCCH_idleRB_fifo_066;
    dlcch_idlerb_list[67] = &Queue_DLCCH_idleRB_fifo_067;
    dlcch_idlerb_list[68] = &Queue_DLCCH_idleRB_fifo_068;
    dlcch_idlerb_list[69] = &Queue_DLCCH_idleRB_fifo_069;
    dlcch_idlerb_list[70] = &Queue_DLCCH_idleRB_fifo_070;
    dlcch_idlerb_list[71] = &Queue_DLCCH_idleRB_fifo_071;
    dlcch_idlerb_list[72] = &Queue_DLCCH_idleRB_fifo_072;
    dlcch_idlerb_list[73] = &Queue_DLCCH_idleRB_fifo_073;
    dlcch_idlerb_list[74] = &Queue_DLCCH_idleRB_fifo_074;
    dlcch_idlerb_list[75] = &Queue_DLCCH_idleRB_fifo_075;
    dlcch_idlerb_list[76] = &Queue_DLCCH_idleRB_fifo_076;
    dlcch_idlerb_list[77] = &Queue_DLCCH_idleRB_fifo_077;
    dlcch_idlerb_list[78] = &Queue_DLCCH_idleRB_fifo_078;
    dlcch_idlerb_list[79] = &Queue_DLCCH_idleRB_fifo_079;
    dlcch_idlerb_list[80] = &Queue_DLCCH_idleRB_fifo_080;
    dlcch_idlerb_list[81] = &Queue_DLCCH_idleRB_fifo_081;
    dlcch_idlerb_list[82] = &Queue_DLCCH_idleRB_fifo_082;
    dlcch_idlerb_list[83] = &Queue_DLCCH_idleRB_fifo_083;
    dlcch_idlerb_list[84] = &Queue_DLCCH_idleRB_fifo_084;
    dlcch_idlerb_list[85] = &Queue_DLCCH_idleRB_fifo_085;
    dlcch_idlerb_list[86] = &Queue_DLCCH_idleRB_fifo_086;
    dlcch_idlerb_list[87] = &Queue_DLCCH_idleRB_fifo_087;
    dlcch_idlerb_list[88] = &Queue_DLCCH_idleRB_fifo_088;
    dlcch_idlerb_list[89] = &Queue_DLCCH_idleRB_fifo_089;
    dlcch_idlerb_list[90] = &Queue_DLCCH_idleRB_fifo_090;
    dlcch_idlerb_list[91] = &Queue_DLCCH_idleRB_fifo_091;
    dlcch_idlerb_list[92] = &Queue_DLCCH_idleRB_fifo_092;
    dlcch_idlerb_list[93] = &Queue_DLCCH_idleRB_fifo_093;
    dlcch_idlerb_list[94] = &Queue_DLCCH_idleRB_fifo_094;
    dlcch_idlerb_list[95] = &Queue_DLCCH_idleRB_fifo_095;
    dlcch_idlerb_list[96] = &Queue_DLCCH_idleRB_fifo_096;
    dlcch_idlerb_list[97] = &Queue_DLCCH_idleRB_fifo_097;
    dlcch_idlerb_list[98] = &Queue_DLCCH_idleRB_fifo_098;
    dlcch_idlerb_list[99] = &Queue_DLCCH_idleRB_fifo_099;
    dlcch_idlerb_list[100] = &Queue_DLCCH_idleRB_fifo_100;
    dlcch_idlerb_list[101] = &Queue_DLCCH_idleRB_fifo_101;
    dlcch_idlerb_list[102] = &Queue_DLCCH_idleRB_fifo_102;
    dlcch_idlerb_list[103] = &Queue_DLCCH_idleRB_fifo_103;
    dlcch_idlerb_list[104] = &Queue_DLCCH_idleRB_fifo_104;
    dlcch_idlerb_list[105] = &Queue_DLCCH_idleRB_fifo_105;
    dlcch_idlerb_list[106] = &Queue_DLCCH_idleRB_fifo_106;
    dlcch_idlerb_list[107] = &Queue_DLCCH_idleRB_fifo_107;
    dlcch_idlerb_list[108] = &Queue_DLCCH_idleRB_fifo_108;
    dlcch_idlerb_list[109] = &Queue_DLCCH_idleRB_fifo_109;
    dlcch_idlerb_list[110] = &Queue_DLCCH_idleRB_fifo_110;
    dlcch_idlerb_list[111] = &Queue_DLCCH_idleRB_fifo_111;
    dlcch_idlerb_list[112] = &Queue_DLCCH_idleRB_fifo_112;
    dlcch_idlerb_list[113] = &Queue_DLCCH_idleRB_fifo_113;
    dlcch_idlerb_list[114] = &Queue_DLCCH_idleRB_fifo_114;
    dlcch_idlerb_list[115] = &Queue_DLCCH_idleRB_fifo_115;
    dlcch_idlerb_list[116] = &Queue_DLCCH_idleRB_fifo_116;
    dlcch_idlerb_list[117] = &Queue_DLCCH_idleRB_fifo_117;
    dlcch_idlerb_list[118] = &Queue_DLCCH_idleRB_fifo_118;
    dlcch_idlerb_list[119] = &Queue_DLCCH_idleRB_fifo_119;
    dlcch_idlerb_list[120] = &Queue_DLCCH_idleRB_fifo_120;
    dlcch_idlerb_list[121] = &Queue_DLCCH_idleRB_fifo_121;
    dlcch_idlerb_list[122] = &Queue_DLCCH_idleRB_fifo_122;
    dlcch_idlerb_list[123] = &Queue_DLCCH_idleRB_fifo_123;
    dlcch_idlerb_list[124] = &Queue_DLCCH_idleRB_fifo_124;
    dlcch_idlerb_list[125] = &Queue_DLCCH_idleRB_fifo_125;
    dlcch_idlerb_list[126] = &Queue_DLCCH_idleRB_fifo_126;
    dlcch_idlerb_list[127] = &Queue_DLCCH_idleRB_fifo_127;
    dlcch_idlerb_list[128] = &Queue_DLCCH_idleRB_fifo_128;
    dlcch_idlerb_list[129] = &Queue_DLCCH_idleRB_fifo_129;
    dlcch_idlerb_list[130] = &Queue_DLCCH_idleRB_fifo_130;
    dlcch_idlerb_list[131] = &Queue_DLCCH_idleRB_fifo_131;
    dlcch_idlerb_list[132] = &Queue_DLCCH_idleRB_fifo_132;
    dlcch_idlerb_list[133] = &Queue_DLCCH_idleRB_fifo_133;
    dlcch_idlerb_list[134] = &Queue_DLCCH_idleRB_fifo_134;
    dlcch_idlerb_list[135] = &Queue_DLCCH_idleRB_fifo_135;
    dlcch_idlerb_list[136] = &Queue_DLCCH_idleRB_fifo_136;
    dlcch_idlerb_list[137] = &Queue_DLCCH_idleRB_fifo_137;
    dlcch_idlerb_list[138] = &Queue_DLCCH_idleRB_fifo_138;
    dlcch_idlerb_list[139] = &Queue_DLCCH_idleRB_fifo_139;
    dlcch_idlerb_list[140] = &Queue_DLCCH_idleRB_fifo_140;
    dlcch_idlerb_list[141] = &Queue_DLCCH_idleRB_fifo_141;
    dlcch_idlerb_list[142] = &Queue_DLCCH_idleRB_fifo_142;
    dlcch_idlerb_list[143] = &Queue_DLCCH_idleRB_fifo_143;
    dlcch_idlerb_list[144] = &Queue_DLCCH_idleRB_fifo_144;
    dlcch_idlerb_list[145] = &Queue_DLCCH_idleRB_fifo_145;
    dlcch_idlerb_list[146] = &Queue_DLCCH_idleRB_fifo_146;
    dlcch_idlerb_list[147] = &Queue_DLCCH_idleRB_fifo_147;
    dlcch_idlerb_list[148] = &Queue_DLCCH_idleRB_fifo_148;
    dlcch_idlerb_list[149] = &Queue_DLCCH_idleRB_fifo_149;

    for (Wave_index; Wave_index < WAVE_NUM; Wave_index++)
    {
        // 初始化控制信道资源列表
        App_queue_init(ulcch_idlerb_list[Wave_index]);
        App_queue_init(dlcch_idlerb_list[Wave_index]);

        for (rb_index; rb_index < RB_TOTALNUM; rb_index++)
        {
            App_queue_push(ulcch_idlerb_list[Wave_index], (char *)&cch_rbid[rb_index], sizeof(u16 *));
            App_queue_push(dlcch_idlerb_list[Wave_index], (char *)&cch_rbid[rb_index], sizeof(u16 *));
        }
        rb_index = 0;
    }
}

// 创建跳波束控制信道空闲资源列表并完成初始化
void init_SCH_idlerb_list(App_queue **ulsch_idlerb_list, App_queue **dlsch_idlerb_list, u16 sch_rbid[][RB_TOTALNUM])
{
    u16 Wave_index = 0;
    u16 rb_index = 0;
    u16 slot_index = 0;

    ulsch_idlerb_list[0] = &Queue_ULSCH_idleRB_fifo_000;
    ulsch_idlerb_list[1] = &Queue_ULSCH_idleRB_fifo_001;
    ulsch_idlerb_list[2] = &Queue_ULSCH_idleRB_fifo_002;
    ulsch_idlerb_list[3] = &Queue_ULSCH_idleRB_fifo_003;
    ulsch_idlerb_list[4] = &Queue_ULSCH_idleRB_fifo_004;
    ulsch_idlerb_list[5] = &Queue_ULSCH_idleRB_fifo_005;
    ulsch_idlerb_list[6] = &Queue_ULSCH_idleRB_fifo_006;
    ulsch_idlerb_list[7] = &Queue_ULSCH_idleRB_fifo_007;
    ulsch_idlerb_list[8] = &Queue_ULSCH_idleRB_fifo_008;
    ulsch_idlerb_list[9] = &Queue_ULSCH_idleRB_fifo_009;
    ulsch_idlerb_list[10] = &Queue_ULSCH_idleRB_fifo_010;
    ulsch_idlerb_list[11] = &Queue_ULSCH_idleRB_fifo_011;
    ulsch_idlerb_list[12] = &Queue_ULSCH_idleRB_fifo_012;
    ulsch_idlerb_list[13] = &Queue_ULSCH_idleRB_fifo_013;
    ulsch_idlerb_list[14] = &Queue_ULSCH_idleRB_fifo_014;
    ulsch_idlerb_list[15] = &Queue_ULSCH_idleRB_fifo_015;
    ulsch_idlerb_list[16] = &Queue_ULSCH_idleRB_fifo_016;
    ulsch_idlerb_list[17] = &Queue_ULSCH_idleRB_fifo_017;
    ulsch_idlerb_list[18] = &Queue_ULSCH_idleRB_fifo_018;
    ulsch_idlerb_list[19] = &Queue_ULSCH_idleRB_fifo_019;
    ulsch_idlerb_list[20] = &Queue_ULSCH_idleRB_fifo_020;
    ulsch_idlerb_list[21] = &Queue_ULSCH_idleRB_fifo_021;
    ulsch_idlerb_list[22] = &Queue_ULSCH_idleRB_fifo_022;
    ulsch_idlerb_list[23] = &Queue_ULSCH_idleRB_fifo_023;
    ulsch_idlerb_list[24] = &Queue_ULSCH_idleRB_fifo_024;
    ulsch_idlerb_list[25] = &Queue_ULSCH_idleRB_fifo_025;
    ulsch_idlerb_list[26] = &Queue_ULSCH_idleRB_fifo_026;
    ulsch_idlerb_list[27] = &Queue_ULSCH_idleRB_fifo_027;
    ulsch_idlerb_list[28] = &Queue_ULSCH_idleRB_fifo_028;
    ulsch_idlerb_list[29] = &Queue_ULSCH_idleRB_fifo_029;
    ulsch_idlerb_list[30] = &Queue_ULSCH_idleRB_fifo_030;
    ulsch_idlerb_list[31] = &Queue_ULSCH_idleRB_fifo_031;
    ulsch_idlerb_list[32] = &Queue_ULSCH_idleRB_fifo_032;
    ulsch_idlerb_list[33] = &Queue_ULSCH_idleRB_fifo_033;
    ulsch_idlerb_list[34] = &Queue_ULSCH_idleRB_fifo_034;
    ulsch_idlerb_list[35] = &Queue_ULSCH_idleRB_fifo_035;
    ulsch_idlerb_list[36] = &Queue_ULSCH_idleRB_fifo_036;
    ulsch_idlerb_list[37] = &Queue_ULSCH_idleRB_fifo_037;
    ulsch_idlerb_list[38] = &Queue_ULSCH_idleRB_fifo_038;
    ulsch_idlerb_list[39] = &Queue_ULSCH_idleRB_fifo_039;
    ulsch_idlerb_list[40] = &Queue_ULSCH_idleRB_fifo_040;
    ulsch_idlerb_list[41] = &Queue_ULSCH_idleRB_fifo_041;
    ulsch_idlerb_list[42] = &Queue_ULSCH_idleRB_fifo_042;
    ulsch_idlerb_list[43] = &Queue_ULSCH_idleRB_fifo_043;
    ulsch_idlerb_list[44] = &Queue_ULSCH_idleRB_fifo_044;
    ulsch_idlerb_list[45] = &Queue_ULSCH_idleRB_fifo_045;
    ulsch_idlerb_list[46] = &Queue_ULSCH_idleRB_fifo_046;
    ulsch_idlerb_list[47] = &Queue_ULSCH_idleRB_fifo_047;
    ulsch_idlerb_list[48] = &Queue_ULSCH_idleRB_fifo_048;
    ulsch_idlerb_list[49] = &Queue_ULSCH_idleRB_fifo_049;
    ulsch_idlerb_list[50] = &Queue_ULSCH_idleRB_fifo_050;
    ulsch_idlerb_list[51] = &Queue_ULSCH_idleRB_fifo_051;
    ulsch_idlerb_list[52] = &Queue_ULSCH_idleRB_fifo_052;
    ulsch_idlerb_list[53] = &Queue_ULSCH_idleRB_fifo_053;
    ulsch_idlerb_list[54] = &Queue_ULSCH_idleRB_fifo_054;
    ulsch_idlerb_list[55] = &Queue_ULSCH_idleRB_fifo_055;
    ulsch_idlerb_list[56] = &Queue_ULSCH_idleRB_fifo_056;
    ulsch_idlerb_list[57] = &Queue_ULSCH_idleRB_fifo_057;
    ulsch_idlerb_list[58] = &Queue_ULSCH_idleRB_fifo_058;
    ulsch_idlerb_list[59] = &Queue_ULSCH_idleRB_fifo_059;
    ulsch_idlerb_list[60] = &Queue_ULSCH_idleRB_fifo_060;
    ulsch_idlerb_list[61] = &Queue_ULSCH_idleRB_fifo_061;
    ulsch_idlerb_list[62] = &Queue_ULSCH_idleRB_fifo_062;
    ulsch_idlerb_list[63] = &Queue_ULSCH_idleRB_fifo_063;
    ulsch_idlerb_list[64] = &Queue_ULSCH_idleRB_fifo_064;
    ulsch_idlerb_list[65] = &Queue_ULSCH_idleRB_fifo_065;
    ulsch_idlerb_list[66] = &Queue_ULSCH_idleRB_fifo_066;
    ulsch_idlerb_list[67] = &Queue_ULSCH_idleRB_fifo_067;
    ulsch_idlerb_list[68] = &Queue_ULSCH_idleRB_fifo_068;
    ulsch_idlerb_list[69] = &Queue_ULSCH_idleRB_fifo_069;
    ulsch_idlerb_list[70] = &Queue_ULSCH_idleRB_fifo_070;
    ulsch_idlerb_list[71] = &Queue_ULSCH_idleRB_fifo_071;
    ulsch_idlerb_list[72] = &Queue_ULSCH_idleRB_fifo_072;
    ulsch_idlerb_list[73] = &Queue_ULSCH_idleRB_fifo_073;
    ulsch_idlerb_list[74] = &Queue_ULSCH_idleRB_fifo_074;
    ulsch_idlerb_list[75] = &Queue_ULSCH_idleRB_fifo_075;
    ulsch_idlerb_list[76] = &Queue_ULSCH_idleRB_fifo_076;
    ulsch_idlerb_list[77] = &Queue_ULSCH_idleRB_fifo_077;
    ulsch_idlerb_list[78] = &Queue_ULSCH_idleRB_fifo_078;
    ulsch_idlerb_list[79] = &Queue_ULSCH_idleRB_fifo_079;
    ulsch_idlerb_list[80] = &Queue_ULSCH_idleRB_fifo_080;
    ulsch_idlerb_list[81] = &Queue_ULSCH_idleRB_fifo_081;
    ulsch_idlerb_list[82] = &Queue_ULSCH_idleRB_fifo_082;
    ulsch_idlerb_list[83] = &Queue_ULSCH_idleRB_fifo_083;
    ulsch_idlerb_list[84] = &Queue_ULSCH_idleRB_fifo_084;
    ulsch_idlerb_list[85] = &Queue_ULSCH_idleRB_fifo_085;
    ulsch_idlerb_list[86] = &Queue_ULSCH_idleRB_fifo_086;
    ulsch_idlerb_list[87] = &Queue_ULSCH_idleRB_fifo_087;
    ulsch_idlerb_list[88] = &Queue_ULSCH_idleRB_fifo_088;
    ulsch_idlerb_list[89] = &Queue_ULSCH_idleRB_fifo_089;
    ulsch_idlerb_list[90] = &Queue_ULSCH_idleRB_fifo_090;
    ulsch_idlerb_list[91] = &Queue_ULSCH_idleRB_fifo_091;
    ulsch_idlerb_list[92] = &Queue_ULSCH_idleRB_fifo_092;
    ulsch_idlerb_list[93] = &Queue_ULSCH_idleRB_fifo_093;
    ulsch_idlerb_list[94] = &Queue_ULSCH_idleRB_fifo_094;
    ulsch_idlerb_list[95] = &Queue_ULSCH_idleRB_fifo_095;
    ulsch_idlerb_list[96] = &Queue_ULSCH_idleRB_fifo_096;
    ulsch_idlerb_list[97] = &Queue_ULSCH_idleRB_fifo_097;
    ulsch_idlerb_list[98] = &Queue_ULSCH_idleRB_fifo_098;
    ulsch_idlerb_list[99] = &Queue_ULSCH_idleRB_fifo_099;
    ulsch_idlerb_list[100] = &Queue_ULSCH_idleRB_fifo_100;
    ulsch_idlerb_list[101] = &Queue_ULSCH_idleRB_fifo_101;
    ulsch_idlerb_list[102] = &Queue_ULSCH_idleRB_fifo_102;
    ulsch_idlerb_list[103] = &Queue_ULSCH_idleRB_fifo_103;
    ulsch_idlerb_list[104] = &Queue_ULSCH_idleRB_fifo_104;
    ulsch_idlerb_list[105] = &Queue_ULSCH_idleRB_fifo_105;
    ulsch_idlerb_list[106] = &Queue_ULSCH_idleRB_fifo_106;
    ulsch_idlerb_list[107] = &Queue_ULSCH_idleRB_fifo_107;
    ulsch_idlerb_list[108] = &Queue_ULSCH_idleRB_fifo_108;
    ulsch_idlerb_list[109] = &Queue_ULSCH_idleRB_fifo_109;
    ulsch_idlerb_list[110] = &Queue_ULSCH_idleRB_fifo_110;
    ulsch_idlerb_list[111] = &Queue_ULSCH_idleRB_fifo_111;
    ulsch_idlerb_list[112] = &Queue_ULSCH_idleRB_fifo_112;
    ulsch_idlerb_list[113] = &Queue_ULSCH_idleRB_fifo_113;
    ulsch_idlerb_list[114] = &Queue_ULSCH_idleRB_fifo_114;
    ulsch_idlerb_list[115] = &Queue_ULSCH_idleRB_fifo_115;
    ulsch_idlerb_list[116] = &Queue_ULSCH_idleRB_fifo_116;
    ulsch_idlerb_list[117] = &Queue_ULSCH_idleRB_fifo_117;
    ulsch_idlerb_list[118] = &Queue_ULSCH_idleRB_fifo_118;
    ulsch_idlerb_list[119] = &Queue_ULSCH_idleRB_fifo_119;
    ulsch_idlerb_list[120] = &Queue_ULSCH_idleRB_fifo_120;
    ulsch_idlerb_list[121] = &Queue_ULSCH_idleRB_fifo_121;
    ulsch_idlerb_list[122] = &Queue_ULSCH_idleRB_fifo_122;
    ulsch_idlerb_list[123] = &Queue_ULSCH_idleRB_fifo_123;
    ulsch_idlerb_list[124] = &Queue_ULSCH_idleRB_fifo_124;
    ulsch_idlerb_list[125] = &Queue_ULSCH_idleRB_fifo_125;
    ulsch_idlerb_list[126] = &Queue_ULSCH_idleRB_fifo_126;
    ulsch_idlerb_list[127] = &Queue_ULSCH_idleRB_fifo_127;
    ulsch_idlerb_list[128] = &Queue_ULSCH_idleRB_fifo_128;
    ulsch_idlerb_list[129] = &Queue_ULSCH_idleRB_fifo_129;
    ulsch_idlerb_list[130] = &Queue_ULSCH_idleRB_fifo_130;
    ulsch_idlerb_list[131] = &Queue_ULSCH_idleRB_fifo_131;
    ulsch_idlerb_list[132] = &Queue_ULSCH_idleRB_fifo_132;
    ulsch_idlerb_list[133] = &Queue_ULSCH_idleRB_fifo_133;
    ulsch_idlerb_list[134] = &Queue_ULSCH_idleRB_fifo_134;
    ulsch_idlerb_list[135] = &Queue_ULSCH_idleRB_fifo_135;
    ulsch_idlerb_list[136] = &Queue_ULSCH_idleRB_fifo_136;
    ulsch_idlerb_list[137] = &Queue_ULSCH_idleRB_fifo_137;
    ulsch_idlerb_list[138] = &Queue_ULSCH_idleRB_fifo_138;
    ulsch_idlerb_list[139] = &Queue_ULSCH_idleRB_fifo_139;
    ulsch_idlerb_list[140] = &Queue_ULSCH_idleRB_fifo_140;
    ulsch_idlerb_list[141] = &Queue_ULSCH_idleRB_fifo_141;
    ulsch_idlerb_list[142] = &Queue_ULSCH_idleRB_fifo_142;
    ulsch_idlerb_list[143] = &Queue_ULSCH_idleRB_fifo_143;
    ulsch_idlerb_list[144] = &Queue_ULSCH_idleRB_fifo_144;
    ulsch_idlerb_list[145] = &Queue_ULSCH_idleRB_fifo_145;
    ulsch_idlerb_list[146] = &Queue_ULSCH_idleRB_fifo_146;
    ulsch_idlerb_list[147] = &Queue_ULSCH_idleRB_fifo_147;
    ulsch_idlerb_list[148] = &Queue_ULSCH_idleRB_fifo_148;
    ulsch_idlerb_list[149] = &Queue_ULSCH_idleRB_fifo_149;

    dlsch_idlerb_list[0] = &Queue_DLSCH_idleRB_fifo_000;
    dlsch_idlerb_list[1] = &Queue_DLSCH_idleRB_fifo_001;
    dlsch_idlerb_list[2] = &Queue_DLSCH_idleRB_fifo_002;
    dlsch_idlerb_list[3] = &Queue_DLSCH_idleRB_fifo_003;
    dlsch_idlerb_list[4] = &Queue_DLSCH_idleRB_fifo_004;
    dlsch_idlerb_list[5] = &Queue_DLSCH_idleRB_fifo_005;
    dlsch_idlerb_list[6] = &Queue_DLSCH_idleRB_fifo_006;
    dlsch_idlerb_list[7] = &Queue_DLSCH_idleRB_fifo_007;
    dlsch_idlerb_list[8] = &Queue_DLSCH_idleRB_fifo_008;
    dlsch_idlerb_list[9] = &Queue_DLSCH_idleRB_fifo_009;
    dlsch_idlerb_list[10] = &Queue_DLSCH_idleRB_fifo_010;
    dlsch_idlerb_list[11] = &Queue_DLSCH_idleRB_fifo_011;
    dlsch_idlerb_list[12] = &Queue_DLSCH_idleRB_fifo_012;
    dlsch_idlerb_list[13] = &Queue_DLSCH_idleRB_fifo_013;
    dlsch_idlerb_list[14] = &Queue_DLSCH_idleRB_fifo_014;
    dlsch_idlerb_list[15] = &Queue_DLSCH_idleRB_fifo_015;
    dlsch_idlerb_list[16] = &Queue_DLSCH_idleRB_fifo_016;
    dlsch_idlerb_list[17] = &Queue_DLSCH_idleRB_fifo_017;
    dlsch_idlerb_list[18] = &Queue_DLSCH_idleRB_fifo_018;
    dlsch_idlerb_list[19] = &Queue_DLSCH_idleRB_fifo_019;
    dlsch_idlerb_list[20] = &Queue_DLSCH_idleRB_fifo_020;
    dlsch_idlerb_list[21] = &Queue_DLSCH_idleRB_fifo_021;
    dlsch_idlerb_list[22] = &Queue_DLSCH_idleRB_fifo_022;
    dlsch_idlerb_list[23] = &Queue_DLSCH_idleRB_fifo_023;
    dlsch_idlerb_list[24] = &Queue_DLSCH_idleRB_fifo_024;
    dlsch_idlerb_list[25] = &Queue_DLSCH_idleRB_fifo_025;
    dlsch_idlerb_list[26] = &Queue_DLSCH_idleRB_fifo_026;
    dlsch_idlerb_list[27] = &Queue_DLSCH_idleRB_fifo_027;
    dlsch_idlerb_list[28] = &Queue_DLSCH_idleRB_fifo_028;
    dlsch_idlerb_list[29] = &Queue_DLSCH_idleRB_fifo_029;
    dlsch_idlerb_list[30] = &Queue_DLSCH_idleRB_fifo_030;
    dlsch_idlerb_list[31] = &Queue_DLSCH_idleRB_fifo_031;
    dlsch_idlerb_list[32] = &Queue_DLSCH_idleRB_fifo_032;
    dlsch_idlerb_list[33] = &Queue_DLSCH_idleRB_fifo_033;
    dlsch_idlerb_list[34] = &Queue_DLSCH_idleRB_fifo_034;
    dlsch_idlerb_list[35] = &Queue_DLSCH_idleRB_fifo_035;
    dlsch_idlerb_list[36] = &Queue_DLSCH_idleRB_fifo_036;
    dlsch_idlerb_list[37] = &Queue_DLSCH_idleRB_fifo_037;
    dlsch_idlerb_list[38] = &Queue_DLSCH_idleRB_fifo_038;
    dlsch_idlerb_list[39] = &Queue_DLSCH_idleRB_fifo_039;
    dlsch_idlerb_list[40] = &Queue_DLSCH_idleRB_fifo_040;
    dlsch_idlerb_list[41] = &Queue_DLSCH_idleRB_fifo_041;
    dlsch_idlerb_list[42] = &Queue_DLSCH_idleRB_fifo_042;
    dlsch_idlerb_list[43] = &Queue_DLSCH_idleRB_fifo_043;
    dlsch_idlerb_list[44] = &Queue_DLSCH_idleRB_fifo_044;
    dlsch_idlerb_list[45] = &Queue_DLSCH_idleRB_fifo_045;
    dlsch_idlerb_list[46] = &Queue_DLSCH_idleRB_fifo_046;
    dlsch_idlerb_list[47] = &Queue_DLSCH_idleRB_fifo_047;
    dlsch_idlerb_list[48] = &Queue_DLSCH_idleRB_fifo_048;
    dlsch_idlerb_list[49] = &Queue_DLSCH_idleRB_fifo_049;
    dlsch_idlerb_list[50] = &Queue_DLSCH_idleRB_fifo_050;
    dlsch_idlerb_list[51] = &Queue_DLSCH_idleRB_fifo_051;
    dlsch_idlerb_list[52] = &Queue_DLSCH_idleRB_fifo_052;
    dlsch_idlerb_list[53] = &Queue_DLSCH_idleRB_fifo_053;
    dlsch_idlerb_list[54] = &Queue_DLSCH_idleRB_fifo_054;
    dlsch_idlerb_list[55] = &Queue_DLSCH_idleRB_fifo_055;
    dlsch_idlerb_list[56] = &Queue_DLSCH_idleRB_fifo_056;
    dlsch_idlerb_list[57] = &Queue_DLSCH_idleRB_fifo_057;
    dlsch_idlerb_list[58] = &Queue_DLSCH_idleRB_fifo_058;
    dlsch_idlerb_list[59] = &Queue_DLSCH_idleRB_fifo_059;
    dlsch_idlerb_list[60] = &Queue_DLSCH_idleRB_fifo_060;
    dlsch_idlerb_list[61] = &Queue_DLSCH_idleRB_fifo_061;
    dlsch_idlerb_list[62] = &Queue_DLSCH_idleRB_fifo_062;
    dlsch_idlerb_list[63] = &Queue_DLSCH_idleRB_fifo_063;
    dlsch_idlerb_list[64] = &Queue_DLSCH_idleRB_fifo_064;
    dlsch_idlerb_list[65] = &Queue_DLSCH_idleRB_fifo_065;
    dlsch_idlerb_list[66] = &Queue_DLSCH_idleRB_fifo_066;
    dlsch_idlerb_list[67] = &Queue_DLSCH_idleRB_fifo_067;
    dlsch_idlerb_list[68] = &Queue_DLSCH_idleRB_fifo_068;
    dlsch_idlerb_list[69] = &Queue_DLSCH_idleRB_fifo_069;
    dlsch_idlerb_list[70] = &Queue_DLSCH_idleRB_fifo_070;
    dlsch_idlerb_list[71] = &Queue_DLSCH_idleRB_fifo_071;
    dlsch_idlerb_list[72] = &Queue_DLSCH_idleRB_fifo_072;
    dlsch_idlerb_list[73] = &Queue_DLSCH_idleRB_fifo_073;
    dlsch_idlerb_list[74] = &Queue_DLSCH_idleRB_fifo_074;
    dlsch_idlerb_list[75] = &Queue_DLSCH_idleRB_fifo_075;
    dlsch_idlerb_list[76] = &Queue_DLSCH_idleRB_fifo_076;
    dlsch_idlerb_list[77] = &Queue_DLSCH_idleRB_fifo_077;
    dlsch_idlerb_list[78] = &Queue_DLSCH_idleRB_fifo_078;
    dlsch_idlerb_list[79] = &Queue_DLSCH_idleRB_fifo_079;
    dlsch_idlerb_list[80] = &Queue_DLSCH_idleRB_fifo_080;
    dlsch_idlerb_list[81] = &Queue_DLSCH_idleRB_fifo_081;
    dlsch_idlerb_list[82] = &Queue_DLSCH_idleRB_fifo_082;
    dlsch_idlerb_list[83] = &Queue_DLSCH_idleRB_fifo_083;
    dlsch_idlerb_list[84] = &Queue_DLSCH_idleRB_fifo_084;
    dlsch_idlerb_list[85] = &Queue_DLSCH_idleRB_fifo_085;
    dlsch_idlerb_list[86] = &Queue_DLSCH_idleRB_fifo_086;
    dlsch_idlerb_list[87] = &Queue_DLSCH_idleRB_fifo_087;
    dlsch_idlerb_list[88] = &Queue_DLSCH_idleRB_fifo_088;
    dlsch_idlerb_list[89] = &Queue_DLSCH_idleRB_fifo_089;
    dlsch_idlerb_list[90] = &Queue_DLSCH_idleRB_fifo_090;
    dlsch_idlerb_list[91] = &Queue_DLSCH_idleRB_fifo_091;
    dlsch_idlerb_list[92] = &Queue_DLSCH_idleRB_fifo_092;
    dlsch_idlerb_list[93] = &Queue_DLSCH_idleRB_fifo_093;
    dlsch_idlerb_list[94] = &Queue_DLSCH_idleRB_fifo_094;
    dlsch_idlerb_list[95] = &Queue_DLSCH_idleRB_fifo_095;
    dlsch_idlerb_list[96] = &Queue_DLSCH_idleRB_fifo_096;
    dlsch_idlerb_list[97] = &Queue_DLSCH_idleRB_fifo_097;
    dlsch_idlerb_list[98] = &Queue_DLSCH_idleRB_fifo_098;
    dlsch_idlerb_list[99] = &Queue_DLSCH_idleRB_fifo_099;
    dlsch_idlerb_list[100] = &Queue_DLSCH_idleRB_fifo_100;
    dlsch_idlerb_list[101] = &Queue_DLSCH_idleRB_fifo_101;
    dlsch_idlerb_list[102] = &Queue_DLSCH_idleRB_fifo_102;
    dlsch_idlerb_list[103] = &Queue_DLSCH_idleRB_fifo_103;
    dlsch_idlerb_list[104] = &Queue_DLSCH_idleRB_fifo_104;
    dlsch_idlerb_list[105] = &Queue_DLSCH_idleRB_fifo_105;
    dlsch_idlerb_list[106] = &Queue_DLSCH_idleRB_fifo_106;
    dlsch_idlerb_list[107] = &Queue_DLSCH_idleRB_fifo_107;
    dlsch_idlerb_list[108] = &Queue_DLSCH_idleRB_fifo_108;
    dlsch_idlerb_list[109] = &Queue_DLSCH_idleRB_fifo_109;
    dlsch_idlerb_list[110] = &Queue_DLSCH_idleRB_fifo_110;
    dlsch_idlerb_list[111] = &Queue_DLSCH_idleRB_fifo_111;
    dlsch_idlerb_list[112] = &Queue_DLSCH_idleRB_fifo_112;
    dlsch_idlerb_list[113] = &Queue_DLSCH_idleRB_fifo_113;
    dlsch_idlerb_list[114] = &Queue_DLSCH_idleRB_fifo_114;
    dlsch_idlerb_list[115] = &Queue_DLSCH_idleRB_fifo_115;
    dlsch_idlerb_list[116] = &Queue_DLSCH_idleRB_fifo_116;
    dlsch_idlerb_list[117] = &Queue_DLSCH_idleRB_fifo_117;
    dlsch_idlerb_list[118] = &Queue_DLSCH_idleRB_fifo_118;
    dlsch_idlerb_list[119] = &Queue_DLSCH_idleRB_fifo_119;
    dlsch_idlerb_list[120] = &Queue_DLSCH_idleRB_fifo_120;
    dlsch_idlerb_list[121] = &Queue_DLSCH_idleRB_fifo_121;
    dlsch_idlerb_list[122] = &Queue_DLSCH_idleRB_fifo_122;
    dlsch_idlerb_list[123] = &Queue_DLSCH_idleRB_fifo_123;
    dlsch_idlerb_list[124] = &Queue_DLSCH_idleRB_fifo_124;
    dlsch_idlerb_list[125] = &Queue_DLSCH_idleRB_fifo_125;
    dlsch_idlerb_list[126] = &Queue_DLSCH_idleRB_fifo_126;
    dlsch_idlerb_list[127] = &Queue_DLSCH_idleRB_fifo_127;
    dlsch_idlerb_list[128] = &Queue_DLSCH_idleRB_fifo_128;
    dlsch_idlerb_list[129] = &Queue_DLSCH_idleRB_fifo_129;
    dlsch_idlerb_list[130] = &Queue_DLSCH_idleRB_fifo_130;
    dlsch_idlerb_list[131] = &Queue_DLSCH_idleRB_fifo_131;
    dlsch_idlerb_list[132] = &Queue_DLSCH_idleRB_fifo_132;
    dlsch_idlerb_list[133] = &Queue_DLSCH_idleRB_fifo_133;
    dlsch_idlerb_list[134] = &Queue_DLSCH_idleRB_fifo_134;
    dlsch_idlerb_list[135] = &Queue_DLSCH_idleRB_fifo_135;
    dlsch_idlerb_list[136] = &Queue_DLSCH_idleRB_fifo_136;
    dlsch_idlerb_list[137] = &Queue_DLSCH_idleRB_fifo_137;
    dlsch_idlerb_list[138] = &Queue_DLSCH_idleRB_fifo_138;
    dlsch_idlerb_list[139] = &Queue_DLSCH_idleRB_fifo_139;
    dlsch_idlerb_list[140] = &Queue_DLSCH_idleRB_fifo_140;
    dlsch_idlerb_list[141] = &Queue_DLSCH_idleRB_fifo_141;
    dlsch_idlerb_list[142] = &Queue_DLSCH_idleRB_fifo_142;
    dlsch_idlerb_list[143] = &Queue_DLSCH_idleRB_fifo_143;
    dlsch_idlerb_list[144] = &Queue_DLSCH_idleRB_fifo_144;
    dlsch_idlerb_list[145] = &Queue_DLSCH_idleRB_fifo_145;
    dlsch_idlerb_list[146] = &Queue_DLSCH_idleRB_fifo_146;
    dlsch_idlerb_list[147] = &Queue_DLSCH_idleRB_fifo_147;
    dlsch_idlerb_list[148] = &Queue_DLSCH_idleRB_fifo_148;
    dlsch_idlerb_list[149] = &Queue_DLSCH_idleRB_fifo_149;

    for (Wave_index; Wave_index < WAVE_NUM; Wave_index++)
    {
        // 初始化控制信道资源列表
        App_queue_init(ulsch_idlerb_list[Wave_index]);
        App_queue_init(dlsch_idlerb_list[Wave_index]);

        for (slot_index; slot_index < SCH_SLOT_NUM; slot_index++)
        {
            for (rb_index; rb_index < RB_TOTALNUM; rb_index++)
            {
                App_queue_push(ulsch_idlerb_list[Wave_index], (char *)&sch_rbid[slot_index][rb_index], sizeof(u16 *));
                App_queue_push(dlsch_idlerb_list[Wave_index], (char *)&sch_rbid[slot_index][rb_index], sizeof(u16 *));
            }
            rb_index = 0;
        }
        slot_index = 0;
    }
}

void init_DL_BusinessPackage_Queue_List(App_queue **dl_queue_list)
{
    u16 Wave_index = 0;

    dl_queue_list[0] = &Queue_DL_BusinessPackage_000;
    dl_queue_list[1] = &Queue_DL_BusinessPackage_001;
    dl_queue_list[2] = &Queue_DL_BusinessPackage_002;
    dl_queue_list[3] = &Queue_DL_BusinessPackage_003;
    dl_queue_list[4] = &Queue_DL_BusinessPackage_004;
    dl_queue_list[5] = &Queue_DL_BusinessPackage_005;
    dl_queue_list[6] = &Queue_DL_BusinessPackage_006;
    dl_queue_list[7] = &Queue_DL_BusinessPackage_007;
    dl_queue_list[8] = &Queue_DL_BusinessPackage_008;
    dl_queue_list[9] = &Queue_DL_BusinessPackage_009;
    dl_queue_list[10] = &Queue_DL_BusinessPackage_010;
    dl_queue_list[11] = &Queue_DL_BusinessPackage_011;
    dl_queue_list[12] = &Queue_DL_BusinessPackage_012;
    dl_queue_list[13] = &Queue_DL_BusinessPackage_013;
    dl_queue_list[14] = &Queue_DL_BusinessPackage_014;
    dl_queue_list[15] = &Queue_DL_BusinessPackage_015;
    dl_queue_list[16] = &Queue_DL_BusinessPackage_016;
    dl_queue_list[17] = &Queue_DL_BusinessPackage_017;
    dl_queue_list[18] = &Queue_DL_BusinessPackage_018;
    dl_queue_list[19] = &Queue_DL_BusinessPackage_019;
    dl_queue_list[20] = &Queue_DL_BusinessPackage_020;
    dl_queue_list[21] = &Queue_DL_BusinessPackage_021;
    dl_queue_list[22] = &Queue_DL_BusinessPackage_022;
    dl_queue_list[23] = &Queue_DL_BusinessPackage_023;
    dl_queue_list[24] = &Queue_DL_BusinessPackage_024;
    dl_queue_list[25] = &Queue_DL_BusinessPackage_025;
    dl_queue_list[26] = &Queue_DL_BusinessPackage_026;
    dl_queue_list[27] = &Queue_DL_BusinessPackage_027;
    dl_queue_list[28] = &Queue_DL_BusinessPackage_028;
    dl_queue_list[29] = &Queue_DL_BusinessPackage_029;
    dl_queue_list[30] = &Queue_DL_BusinessPackage_030;
    dl_queue_list[31] = &Queue_DL_BusinessPackage_031;
    dl_queue_list[32] = &Queue_DL_BusinessPackage_032;
    dl_queue_list[33] = &Queue_DL_BusinessPackage_033;
    dl_queue_list[34] = &Queue_DL_BusinessPackage_034;
    dl_queue_list[35] = &Queue_DL_BusinessPackage_035;
    dl_queue_list[36] = &Queue_DL_BusinessPackage_036;
    dl_queue_list[37] = &Queue_DL_BusinessPackage_037;
    dl_queue_list[38] = &Queue_DL_BusinessPackage_038;
    dl_queue_list[39] = &Queue_DL_BusinessPackage_039;
    dl_queue_list[40] = &Queue_DL_BusinessPackage_040;
    dl_queue_list[41] = &Queue_DL_BusinessPackage_041;
    dl_queue_list[42] = &Queue_DL_BusinessPackage_042;
    dl_queue_list[43] = &Queue_DL_BusinessPackage_043;
    dl_queue_list[44] = &Queue_DL_BusinessPackage_044;
    dl_queue_list[45] = &Queue_DL_BusinessPackage_045;
    dl_queue_list[46] = &Queue_DL_BusinessPackage_046;
    dl_queue_list[47] = &Queue_DL_BusinessPackage_047;
    dl_queue_list[48] = &Queue_DL_BusinessPackage_048;
    dl_queue_list[49] = &Queue_DL_BusinessPackage_049;
    dl_queue_list[50] = &Queue_DL_BusinessPackage_050;
    dl_queue_list[51] = &Queue_DL_BusinessPackage_051;
    dl_queue_list[52] = &Queue_DL_BusinessPackage_052;
    dl_queue_list[53] = &Queue_DL_BusinessPackage_053;
    dl_queue_list[54] = &Queue_DL_BusinessPackage_054;
    dl_queue_list[55] = &Queue_DL_BusinessPackage_055;
    dl_queue_list[56] = &Queue_DL_BusinessPackage_056;
    dl_queue_list[57] = &Queue_DL_BusinessPackage_057;
    dl_queue_list[58] = &Queue_DL_BusinessPackage_058;
    dl_queue_list[59] = &Queue_DL_BusinessPackage_059;
    dl_queue_list[60] = &Queue_DL_BusinessPackage_060;
    dl_queue_list[61] = &Queue_DL_BusinessPackage_061;
    dl_queue_list[62] = &Queue_DL_BusinessPackage_062;
    dl_queue_list[63] = &Queue_DL_BusinessPackage_063;
    dl_queue_list[64] = &Queue_DL_BusinessPackage_064;
    dl_queue_list[65] = &Queue_DL_BusinessPackage_065;
    dl_queue_list[66] = &Queue_DL_BusinessPackage_066;
    dl_queue_list[67] = &Queue_DL_BusinessPackage_067;
    dl_queue_list[68] = &Queue_DL_BusinessPackage_068;
    dl_queue_list[69] = &Queue_DL_BusinessPackage_069;
    dl_queue_list[70] = &Queue_DL_BusinessPackage_070;
    dl_queue_list[71] = &Queue_DL_BusinessPackage_071;
    dl_queue_list[72] = &Queue_DL_BusinessPackage_072;
    dl_queue_list[73] = &Queue_DL_BusinessPackage_073;
    dl_queue_list[74] = &Queue_DL_BusinessPackage_074;
    dl_queue_list[75] = &Queue_DL_BusinessPackage_075;
    dl_queue_list[76] = &Queue_DL_BusinessPackage_076;
    dl_queue_list[77] = &Queue_DL_BusinessPackage_077;
    dl_queue_list[78] = &Queue_DL_BusinessPackage_078;
    dl_queue_list[79] = &Queue_DL_BusinessPackage_079;
    dl_queue_list[80] = &Queue_DL_BusinessPackage_080;
    dl_queue_list[81] = &Queue_DL_BusinessPackage_081;
    dl_queue_list[82] = &Queue_DL_BusinessPackage_082;
    dl_queue_list[83] = &Queue_DL_BusinessPackage_083;
    dl_queue_list[84] = &Queue_DL_BusinessPackage_084;
    dl_queue_list[85] = &Queue_DL_BusinessPackage_085;
    dl_queue_list[86] = &Queue_DL_BusinessPackage_086;
    dl_queue_list[87] = &Queue_DL_BusinessPackage_087;
    dl_queue_list[88] = &Queue_DL_BusinessPackage_088;
    dl_queue_list[89] = &Queue_DL_BusinessPackage_089;
    dl_queue_list[90] = &Queue_DL_BusinessPackage_090;
    dl_queue_list[91] = &Queue_DL_BusinessPackage_091;
    dl_queue_list[92] = &Queue_DL_BusinessPackage_092;
    dl_queue_list[93] = &Queue_DL_BusinessPackage_093;
    dl_queue_list[94] = &Queue_DL_BusinessPackage_094;
    dl_queue_list[95] = &Queue_DL_BusinessPackage_095;
    dl_queue_list[96] = &Queue_DL_BusinessPackage_096;
    dl_queue_list[97] = &Queue_DL_BusinessPackage_097;
    dl_queue_list[98] = &Queue_DL_BusinessPackage_098;
    dl_queue_list[99] = &Queue_DL_BusinessPackage_099;
    dl_queue_list[100] = &Queue_DL_BusinessPackage_100;
    dl_queue_list[101] = &Queue_DL_BusinessPackage_101;
    dl_queue_list[102] = &Queue_DL_BusinessPackage_102;
    dl_queue_list[103] = &Queue_DL_BusinessPackage_103;
    dl_queue_list[104] = &Queue_DL_BusinessPackage_104;
    dl_queue_list[105] = &Queue_DL_BusinessPackage_105;
    dl_queue_list[106] = &Queue_DL_BusinessPackage_106;
    dl_queue_list[107] = &Queue_DL_BusinessPackage_107;
    dl_queue_list[108] = &Queue_DL_BusinessPackage_108;
    dl_queue_list[109] = &Queue_DL_BusinessPackage_109;
    dl_queue_list[110] = &Queue_DL_BusinessPackage_110;
    dl_queue_list[111] = &Queue_DL_BusinessPackage_111;
    dl_queue_list[112] = &Queue_DL_BusinessPackage_112;
    dl_queue_list[113] = &Queue_DL_BusinessPackage_113;
    dl_queue_list[114] = &Queue_DL_BusinessPackage_114;
    dl_queue_list[115] = &Queue_DL_BusinessPackage_115;
    dl_queue_list[116] = &Queue_DL_BusinessPackage_116;
    dl_queue_list[117] = &Queue_DL_BusinessPackage_117;
    dl_queue_list[118] = &Queue_DL_BusinessPackage_118;
    dl_queue_list[119] = &Queue_DL_BusinessPackage_119;
    dl_queue_list[120] = &Queue_DL_BusinessPackage_120;
    dl_queue_list[121] = &Queue_DL_BusinessPackage_121;
    dl_queue_list[122] = &Queue_DL_BusinessPackage_122;
    dl_queue_list[123] = &Queue_DL_BusinessPackage_123;
    dl_queue_list[124] = &Queue_DL_BusinessPackage_124;
    dl_queue_list[125] = &Queue_DL_BusinessPackage_125;
    dl_queue_list[126] = &Queue_DL_BusinessPackage_126;
    dl_queue_list[127] = &Queue_DL_BusinessPackage_127;
    dl_queue_list[128] = &Queue_DL_BusinessPackage_128;
    dl_queue_list[129] = &Queue_DL_BusinessPackage_129;
    dl_queue_list[130] = &Queue_DL_BusinessPackage_130;
    dl_queue_list[131] = &Queue_DL_BusinessPackage_131;
    dl_queue_list[132] = &Queue_DL_BusinessPackage_132;
    dl_queue_list[133] = &Queue_DL_BusinessPackage_133;
    dl_queue_list[134] = &Queue_DL_BusinessPackage_134;
    dl_queue_list[135] = &Queue_DL_BusinessPackage_135;
    dl_queue_list[136] = &Queue_DL_BusinessPackage_136;
    dl_queue_list[137] = &Queue_DL_BusinessPackage_137;
    dl_queue_list[138] = &Queue_DL_BusinessPackage_138;
    dl_queue_list[139] = &Queue_DL_BusinessPackage_139;
    dl_queue_list[140] = &Queue_DL_BusinessPackage_140;
    dl_queue_list[141] = &Queue_DL_BusinessPackage_141;
    dl_queue_list[142] = &Queue_DL_BusinessPackage_142;
    dl_queue_list[143] = &Queue_DL_BusinessPackage_143;
    dl_queue_list[144] = &Queue_DL_BusinessPackage_144;
    dl_queue_list[145] = &Queue_DL_BusinessPackage_145;
    dl_queue_list[146] = &Queue_DL_BusinessPackage_146;
    dl_queue_list[147] = &Queue_DL_BusinessPackage_147;
    dl_queue_list[148] = &Queue_DL_BusinessPackage_148;
    dl_queue_list[149] = &Queue_DL_BusinessPackage_149;

    for (Wave_index; Wave_index < WAVE_NUM; Wave_index++)
    {
        App_queue_init(dl_queue_list[Wave_index]);
    }
}

void init_DL_ControlPackage_Queue_List(App_queue **dl_queue_list)
{
    u16 Wave_index = 0;

    dl_queue_list[0] = &Queue_DL_ControlPackage_000;
    dl_queue_list[1] = &Queue_DL_ControlPackage_001;
    dl_queue_list[2] = &Queue_DL_ControlPackage_002;
    dl_queue_list[3] = &Queue_DL_ControlPackage_003;
    dl_queue_list[4] = &Queue_DL_ControlPackage_004;
    dl_queue_list[5] = &Queue_DL_ControlPackage_005;
    dl_queue_list[6] = &Queue_DL_ControlPackage_006;
    dl_queue_list[7] = &Queue_DL_ControlPackage_007;
    dl_queue_list[8] = &Queue_DL_ControlPackage_008;
    dl_queue_list[9] = &Queue_DL_ControlPackage_009;
    dl_queue_list[10] = &Queue_DL_ControlPackage_010;
    dl_queue_list[11] = &Queue_DL_ControlPackage_011;
    dl_queue_list[12] = &Queue_DL_ControlPackage_012;
    dl_queue_list[13] = &Queue_DL_ControlPackage_013;
    dl_queue_list[14] = &Queue_DL_ControlPackage_014;
    dl_queue_list[15] = &Queue_DL_ControlPackage_015;
    dl_queue_list[16] = &Queue_DL_ControlPackage_016;
    dl_queue_list[17] = &Queue_DL_ControlPackage_017;
    dl_queue_list[18] = &Queue_DL_ControlPackage_018;
    dl_queue_list[19] = &Queue_DL_ControlPackage_019;
    dl_queue_list[20] = &Queue_DL_ControlPackage_020;
    dl_queue_list[21] = &Queue_DL_ControlPackage_021;
    dl_queue_list[22] = &Queue_DL_ControlPackage_022;
    dl_queue_list[23] = &Queue_DL_ControlPackage_023;
    dl_queue_list[24] = &Queue_DL_ControlPackage_024;
    dl_queue_list[25] = &Queue_DL_ControlPackage_025;
    dl_queue_list[26] = &Queue_DL_ControlPackage_026;
    dl_queue_list[27] = &Queue_DL_ControlPackage_027;
    dl_queue_list[28] = &Queue_DL_ControlPackage_028;
    dl_queue_list[29] = &Queue_DL_ControlPackage_029;
    dl_queue_list[30] = &Queue_DL_ControlPackage_030;
    dl_queue_list[31] = &Queue_DL_ControlPackage_031;
    dl_queue_list[32] = &Queue_DL_ControlPackage_032;
    dl_queue_list[33] = &Queue_DL_ControlPackage_033;
    dl_queue_list[34] = &Queue_DL_ControlPackage_034;
    dl_queue_list[35] = &Queue_DL_ControlPackage_035;
    dl_queue_list[36] = &Queue_DL_ControlPackage_036;
    dl_queue_list[37] = &Queue_DL_ControlPackage_037;
    dl_queue_list[38] = &Queue_DL_ControlPackage_038;
    dl_queue_list[39] = &Queue_DL_ControlPackage_039;
    dl_queue_list[40] = &Queue_DL_ControlPackage_040;
    dl_queue_list[41] = &Queue_DL_ControlPackage_041;
    dl_queue_list[42] = &Queue_DL_ControlPackage_042;
    dl_queue_list[43] = &Queue_DL_ControlPackage_043;
    dl_queue_list[44] = &Queue_DL_ControlPackage_044;
    dl_queue_list[45] = &Queue_DL_ControlPackage_045;
    dl_queue_list[46] = &Queue_DL_ControlPackage_046;
    dl_queue_list[47] = &Queue_DL_ControlPackage_047;
    dl_queue_list[48] = &Queue_DL_ControlPackage_048;
    dl_queue_list[49] = &Queue_DL_ControlPackage_049;
    dl_queue_list[50] = &Queue_DL_ControlPackage_050;
    dl_queue_list[51] = &Queue_DL_ControlPackage_051;
    dl_queue_list[52] = &Queue_DL_ControlPackage_052;
    dl_queue_list[53] = &Queue_DL_ControlPackage_053;
    dl_queue_list[54] = &Queue_DL_ControlPackage_054;
    dl_queue_list[55] = &Queue_DL_ControlPackage_055;
    dl_queue_list[56] = &Queue_DL_ControlPackage_056;
    dl_queue_list[57] = &Queue_DL_ControlPackage_057;
    dl_queue_list[58] = &Queue_DL_ControlPackage_058;
    dl_queue_list[59] = &Queue_DL_ControlPackage_059;
    dl_queue_list[60] = &Queue_DL_ControlPackage_060;
    dl_queue_list[61] = &Queue_DL_ControlPackage_061;
    dl_queue_list[62] = &Queue_DL_ControlPackage_062;
    dl_queue_list[63] = &Queue_DL_ControlPackage_063;
    dl_queue_list[64] = &Queue_DL_ControlPackage_064;
    dl_queue_list[65] = &Queue_DL_ControlPackage_065;
    dl_queue_list[66] = &Queue_DL_ControlPackage_066;
    dl_queue_list[67] = &Queue_DL_ControlPackage_067;
    dl_queue_list[68] = &Queue_DL_ControlPackage_068;
    dl_queue_list[69] = &Queue_DL_ControlPackage_069;
    dl_queue_list[70] = &Queue_DL_ControlPackage_070;
    dl_queue_list[71] = &Queue_DL_ControlPackage_071;
    dl_queue_list[72] = &Queue_DL_ControlPackage_072;
    dl_queue_list[73] = &Queue_DL_ControlPackage_073;
    dl_queue_list[74] = &Queue_DL_ControlPackage_074;
    dl_queue_list[75] = &Queue_DL_ControlPackage_075;
    dl_queue_list[76] = &Queue_DL_ControlPackage_076;
    dl_queue_list[77] = &Queue_DL_ControlPackage_077;
    dl_queue_list[78] = &Queue_DL_ControlPackage_078;
    dl_queue_list[79] = &Queue_DL_ControlPackage_079;
    dl_queue_list[80] = &Queue_DL_ControlPackage_080;
    dl_queue_list[81] = &Queue_DL_ControlPackage_081;
    dl_queue_list[82] = &Queue_DL_ControlPackage_082;
    dl_queue_list[83] = &Queue_DL_ControlPackage_083;
    dl_queue_list[84] = &Queue_DL_ControlPackage_084;
    dl_queue_list[85] = &Queue_DL_ControlPackage_085;
    dl_queue_list[86] = &Queue_DL_ControlPackage_086;
    dl_queue_list[87] = &Queue_DL_ControlPackage_087;
    dl_queue_list[88] = &Queue_DL_ControlPackage_088;
    dl_queue_list[89] = &Queue_DL_ControlPackage_089;
    dl_queue_list[90] = &Queue_DL_ControlPackage_090;
    dl_queue_list[91] = &Queue_DL_ControlPackage_091;
    dl_queue_list[92] = &Queue_DL_ControlPackage_092;
    dl_queue_list[93] = &Queue_DL_ControlPackage_093;
    dl_queue_list[94] = &Queue_DL_ControlPackage_094;
    dl_queue_list[95] = &Queue_DL_ControlPackage_095;
    dl_queue_list[96] = &Queue_DL_ControlPackage_096;
    dl_queue_list[97] = &Queue_DL_ControlPackage_097;
    dl_queue_list[98] = &Queue_DL_ControlPackage_098;
    dl_queue_list[99] = &Queue_DL_ControlPackage_099;
    dl_queue_list[100] = &Queue_DL_ControlPackage_100;
    dl_queue_list[101] = &Queue_DL_ControlPackage_101;
    dl_queue_list[102] = &Queue_DL_ControlPackage_102;
    dl_queue_list[103] = &Queue_DL_ControlPackage_103;
    dl_queue_list[104] = &Queue_DL_ControlPackage_104;
    dl_queue_list[105] = &Queue_DL_ControlPackage_105;
    dl_queue_list[106] = &Queue_DL_ControlPackage_106;
    dl_queue_list[107] = &Queue_DL_ControlPackage_107;
    dl_queue_list[108] = &Queue_DL_ControlPackage_108;
    dl_queue_list[109] = &Queue_DL_ControlPackage_109;
    dl_queue_list[110] = &Queue_DL_ControlPackage_110;
    dl_queue_list[111] = &Queue_DL_ControlPackage_111;
    dl_queue_list[112] = &Queue_DL_ControlPackage_112;
    dl_queue_list[113] = &Queue_DL_ControlPackage_113;
    dl_queue_list[114] = &Queue_DL_ControlPackage_114;
    dl_queue_list[115] = &Queue_DL_ControlPackage_115;
    dl_queue_list[116] = &Queue_DL_ControlPackage_116;
    dl_queue_list[117] = &Queue_DL_ControlPackage_117;
    dl_queue_list[118] = &Queue_DL_ControlPackage_118;
    dl_queue_list[119] = &Queue_DL_ControlPackage_119;
    dl_queue_list[120] = &Queue_DL_ControlPackage_120;
    dl_queue_list[121] = &Queue_DL_ControlPackage_121;
    dl_queue_list[122] = &Queue_DL_ControlPackage_122;
    dl_queue_list[123] = &Queue_DL_ControlPackage_123;
    dl_queue_list[124] = &Queue_DL_ControlPackage_124;
    dl_queue_list[125] = &Queue_DL_ControlPackage_125;
    dl_queue_list[126] = &Queue_DL_ControlPackage_126;
    dl_queue_list[127] = &Queue_DL_ControlPackage_127;
    dl_queue_list[128] = &Queue_DL_ControlPackage_128;
    dl_queue_list[129] = &Queue_DL_ControlPackage_129;
    dl_queue_list[130] = &Queue_DL_ControlPackage_130;
    dl_queue_list[131] = &Queue_DL_ControlPackage_131;
    dl_queue_list[132] = &Queue_DL_ControlPackage_132;
    dl_queue_list[133] = &Queue_DL_ControlPackage_133;
    dl_queue_list[134] = &Queue_DL_ControlPackage_134;
    dl_queue_list[135] = &Queue_DL_ControlPackage_135;
    dl_queue_list[136] = &Queue_DL_ControlPackage_136;
    dl_queue_list[137] = &Queue_DL_ControlPackage_137;
    dl_queue_list[138] = &Queue_DL_ControlPackage_138;
    dl_queue_list[139] = &Queue_DL_ControlPackage_139;
    dl_queue_list[140] = &Queue_DL_ControlPackage_140;
    dl_queue_list[141] = &Queue_DL_ControlPackage_141;
    dl_queue_list[142] = &Queue_DL_ControlPackage_142;
    dl_queue_list[143] = &Queue_DL_ControlPackage_143;
    dl_queue_list[144] = &Queue_DL_ControlPackage_144;
    dl_queue_list[145] = &Queue_DL_ControlPackage_145;
    dl_queue_list[146] = &Queue_DL_ControlPackage_146;
    dl_queue_list[147] = &Queue_DL_ControlPackage_147;
    dl_queue_list[148] = &Queue_DL_ControlPackage_148;
    dl_queue_list[149] = &Queue_DL_ControlPackage_149;

    for (Wave_index; Wave_index < WAVE_NUM; Wave_index++)
    {
        App_queue_init(dl_queue_list[Wave_index]);
    }
}
// 下行业务缓存虚波位与用户队列的映射的指针数组的初始化
void wave_ueser_mapPtrArray_init(UserInfo_Block (*PtrArray)[PER_WAVE_MAX_NUM], u32 x_size)
{
    u32 x_index;
    u32 y_index;

    x_index = 0;
    for (x_index; x_index < x_size; x_index++)
    {
        y_index;
        for (y_index; y_index < PER_WAVE_MAX_NUM; y_index++)
        {
            PtrArray[x_index][y_index].State = 0;
            PtrArray[x_index][y_index].U_RNTI = 0xFFFFFFFF;
            PtrArray[x_index][y_index].UE_Level = 0xFF;
            PtrArray[x_index][y_index].MCS = 0xFFFFFFFF;
            PtrArray[x_index][y_index].User_DataQueue_ptr = NULL;
        }
    }
}
// 记录每个虚波位接入了多少用户的数组的初始化
void per_wave_usernum_init(u8 *array, u32 size)
{
    u32 index;

    index = 0;
    for (index; index < size; index++)
    {
        array[index] = 0;
    }
}
// 缓存下行业务数据的vector的初始化函数
void init_DL_data_buffer_Cvector_List()
{
    DL_data_buffer_Cvector_List[0] = &DL_data_buffer_Cvector_000;
    DL_data_buffer_Cvector_List[1] = &DL_data_buffer_Cvector_001;
    DL_data_buffer_Cvector_List[2] = &DL_data_buffer_Cvector_002;
    DL_data_buffer_Cvector_List[3] = &DL_data_buffer_Cvector_003;
    DL_data_buffer_Cvector_List[4] = &DL_data_buffer_Cvector_004;
    DL_data_buffer_Cvector_List[5] = &DL_data_buffer_Cvector_005;
    DL_data_buffer_Cvector_List[6] = &DL_data_buffer_Cvector_006;
    DL_data_buffer_Cvector_List[7] = &DL_data_buffer_Cvector_007;
    DL_data_buffer_Cvector_List[8] = &DL_data_buffer_Cvector_008;
    DL_data_buffer_Cvector_List[9] = &DL_data_buffer_Cvector_009;
    DL_data_buffer_Cvector_List[10] = &DL_data_buffer_Cvector_010;
    DL_data_buffer_Cvector_List[11] = &DL_data_buffer_Cvector_011;
    DL_data_buffer_Cvector_List[12] = &DL_data_buffer_Cvector_012;
    DL_data_buffer_Cvector_List[13] = &DL_data_buffer_Cvector_013;
    DL_data_buffer_Cvector_List[14] = &DL_data_buffer_Cvector_014;
    DL_data_buffer_Cvector_List[15] = &DL_data_buffer_Cvector_015;
    DL_data_buffer_Cvector_List[16] = &DL_data_buffer_Cvector_016;
    DL_data_buffer_Cvector_List[17] = &DL_data_buffer_Cvector_017;
    DL_data_buffer_Cvector_List[18] = &DL_data_buffer_Cvector_018;
    DL_data_buffer_Cvector_List[19] = &DL_data_buffer_Cvector_019;
    DL_data_buffer_Cvector_List[20] = &DL_data_buffer_Cvector_020;
    DL_data_buffer_Cvector_List[21] = &DL_data_buffer_Cvector_021;
    DL_data_buffer_Cvector_List[22] = &DL_data_buffer_Cvector_022;
    DL_data_buffer_Cvector_List[23] = &DL_data_buffer_Cvector_023;
    DL_data_buffer_Cvector_List[24] = &DL_data_buffer_Cvector_024;
    DL_data_buffer_Cvector_List[25] = &DL_data_buffer_Cvector_025;
    DL_data_buffer_Cvector_List[26] = &DL_data_buffer_Cvector_026;
    DL_data_buffer_Cvector_List[27] = &DL_data_buffer_Cvector_027;
    DL_data_buffer_Cvector_List[28] = &DL_data_buffer_Cvector_028;
    DL_data_buffer_Cvector_List[29] = &DL_data_buffer_Cvector_029;
    DL_data_buffer_Cvector_List[30] = &DL_data_buffer_Cvector_030;
    DL_data_buffer_Cvector_List[31] = &DL_data_buffer_Cvector_031;
    DL_data_buffer_Cvector_List[32] = &DL_data_buffer_Cvector_032;
    DL_data_buffer_Cvector_List[33] = &DL_data_buffer_Cvector_033;
    DL_data_buffer_Cvector_List[34] = &DL_data_buffer_Cvector_034;
    DL_data_buffer_Cvector_List[35] = &DL_data_buffer_Cvector_035;
    DL_data_buffer_Cvector_List[36] = &DL_data_buffer_Cvector_036;
    DL_data_buffer_Cvector_List[37] = &DL_data_buffer_Cvector_037;
    DL_data_buffer_Cvector_List[38] = &DL_data_buffer_Cvector_038;
    DL_data_buffer_Cvector_List[39] = &DL_data_buffer_Cvector_039;
    DL_data_buffer_Cvector_List[40] = &DL_data_buffer_Cvector_040;
    DL_data_buffer_Cvector_List[41] = &DL_data_buffer_Cvector_041;
    DL_data_buffer_Cvector_List[42] = &DL_data_buffer_Cvector_042;
    DL_data_buffer_Cvector_List[43] = &DL_data_buffer_Cvector_043;
    DL_data_buffer_Cvector_List[44] = &DL_data_buffer_Cvector_044;
    DL_data_buffer_Cvector_List[45] = &DL_data_buffer_Cvector_045;
    DL_data_buffer_Cvector_List[46] = &DL_data_buffer_Cvector_046;
    DL_data_buffer_Cvector_List[47] = &DL_data_buffer_Cvector_047;
    DL_data_buffer_Cvector_List[48] = &DL_data_buffer_Cvector_048;
    DL_data_buffer_Cvector_List[49] = &DL_data_buffer_Cvector_049;
    DL_data_buffer_Cvector_List[50] = &DL_data_buffer_Cvector_050;
    DL_data_buffer_Cvector_List[51] = &DL_data_buffer_Cvector_051;
    DL_data_buffer_Cvector_List[52] = &DL_data_buffer_Cvector_052;
    DL_data_buffer_Cvector_List[53] = &DL_data_buffer_Cvector_053;
    DL_data_buffer_Cvector_List[54] = &DL_data_buffer_Cvector_054;
    DL_data_buffer_Cvector_List[55] = &DL_data_buffer_Cvector_055;
    DL_data_buffer_Cvector_List[56] = &DL_data_buffer_Cvector_056;
    DL_data_buffer_Cvector_List[57] = &DL_data_buffer_Cvector_057;
    DL_data_buffer_Cvector_List[58] = &DL_data_buffer_Cvector_058;
    DL_data_buffer_Cvector_List[59] = &DL_data_buffer_Cvector_059;
    DL_data_buffer_Cvector_List[60] = &DL_data_buffer_Cvector_060;
    DL_data_buffer_Cvector_List[61] = &DL_data_buffer_Cvector_061;
    DL_data_buffer_Cvector_List[62] = &DL_data_buffer_Cvector_062;
    DL_data_buffer_Cvector_List[63] = &DL_data_buffer_Cvector_063;
    DL_data_buffer_Cvector_List[64] = &DL_data_buffer_Cvector_064;
    DL_data_buffer_Cvector_List[65] = &DL_data_buffer_Cvector_065;
    DL_data_buffer_Cvector_List[66] = &DL_data_buffer_Cvector_066;
    DL_data_buffer_Cvector_List[67] = &DL_data_buffer_Cvector_067;
    DL_data_buffer_Cvector_List[68] = &DL_data_buffer_Cvector_068;
    DL_data_buffer_Cvector_List[69] = &DL_data_buffer_Cvector_069;
    DL_data_buffer_Cvector_List[70] = &DL_data_buffer_Cvector_070;
    DL_data_buffer_Cvector_List[71] = &DL_data_buffer_Cvector_071;
    DL_data_buffer_Cvector_List[72] = &DL_data_buffer_Cvector_072;
    DL_data_buffer_Cvector_List[73] = &DL_data_buffer_Cvector_073;
    DL_data_buffer_Cvector_List[74] = &DL_data_buffer_Cvector_074;
    DL_data_buffer_Cvector_List[75] = &DL_data_buffer_Cvector_075;
    DL_data_buffer_Cvector_List[76] = &DL_data_buffer_Cvector_076;
    DL_data_buffer_Cvector_List[77] = &DL_data_buffer_Cvector_077;
    DL_data_buffer_Cvector_List[78] = &DL_data_buffer_Cvector_078;
    DL_data_buffer_Cvector_List[79] = &DL_data_buffer_Cvector_079;
    DL_data_buffer_Cvector_List[80] = &DL_data_buffer_Cvector_080;
    DL_data_buffer_Cvector_List[81] = &DL_data_buffer_Cvector_081;
    DL_data_buffer_Cvector_List[82] = &DL_data_buffer_Cvector_082;
    DL_data_buffer_Cvector_List[83] = &DL_data_buffer_Cvector_083;
    DL_data_buffer_Cvector_List[84] = &DL_data_buffer_Cvector_084;
    DL_data_buffer_Cvector_List[85] = &DL_data_buffer_Cvector_085;
    DL_data_buffer_Cvector_List[86] = &DL_data_buffer_Cvector_086;
    DL_data_buffer_Cvector_List[87] = &DL_data_buffer_Cvector_087;
    DL_data_buffer_Cvector_List[88] = &DL_data_buffer_Cvector_088;
    DL_data_buffer_Cvector_List[89] = &DL_data_buffer_Cvector_089;
    DL_data_buffer_Cvector_List[90] = &DL_data_buffer_Cvector_090;
    DL_data_buffer_Cvector_List[91] = &DL_data_buffer_Cvector_091;
    DL_data_buffer_Cvector_List[92] = &DL_data_buffer_Cvector_092;
    DL_data_buffer_Cvector_List[93] = &DL_data_buffer_Cvector_093;
    DL_data_buffer_Cvector_List[94] = &DL_data_buffer_Cvector_094;
    DL_data_buffer_Cvector_List[95] = &DL_data_buffer_Cvector_095;
    DL_data_buffer_Cvector_List[96] = &DL_data_buffer_Cvector_096;
    DL_data_buffer_Cvector_List[97] = &DL_data_buffer_Cvector_097;
    DL_data_buffer_Cvector_List[98] = &DL_data_buffer_Cvector_098;
    DL_data_buffer_Cvector_List[99] = &DL_data_buffer_Cvector_099;
    DL_data_buffer_Cvector_List[100] = &DL_data_buffer_Cvector_100;
    DL_data_buffer_Cvector_List[101] = &DL_data_buffer_Cvector_101;
    DL_data_buffer_Cvector_List[102] = &DL_data_buffer_Cvector_102;
    DL_data_buffer_Cvector_List[103] = &DL_data_buffer_Cvector_103;
    DL_data_buffer_Cvector_List[104] = &DL_data_buffer_Cvector_104;
    DL_data_buffer_Cvector_List[105] = &DL_data_buffer_Cvector_105;
    DL_data_buffer_Cvector_List[106] = &DL_data_buffer_Cvector_106;
    DL_data_buffer_Cvector_List[107] = &DL_data_buffer_Cvector_107;
    DL_data_buffer_Cvector_List[108] = &DL_data_buffer_Cvector_108;
    DL_data_buffer_Cvector_List[109] = &DL_data_buffer_Cvector_109;
    DL_data_buffer_Cvector_List[110] = &DL_data_buffer_Cvector_110;
    DL_data_buffer_Cvector_List[111] = &DL_data_buffer_Cvector_111;
    DL_data_buffer_Cvector_List[112] = &DL_data_buffer_Cvector_112;
    DL_data_buffer_Cvector_List[113] = &DL_data_buffer_Cvector_113;
    DL_data_buffer_Cvector_List[114] = &DL_data_buffer_Cvector_114;
    DL_data_buffer_Cvector_List[115] = &DL_data_buffer_Cvector_115;
    DL_data_buffer_Cvector_List[116] = &DL_data_buffer_Cvector_116;
    DL_data_buffer_Cvector_List[117] = &DL_data_buffer_Cvector_117;
    DL_data_buffer_Cvector_List[118] = &DL_data_buffer_Cvector_118;
    DL_data_buffer_Cvector_List[119] = &DL_data_buffer_Cvector_119;
    DL_data_buffer_Cvector_List[120] = &DL_data_buffer_Cvector_120;
    DL_data_buffer_Cvector_List[121] = &DL_data_buffer_Cvector_121;
    DL_data_buffer_Cvector_List[122] = &DL_data_buffer_Cvector_122;
    DL_data_buffer_Cvector_List[123] = &DL_data_buffer_Cvector_123;
    DL_data_buffer_Cvector_List[124] = &DL_data_buffer_Cvector_124;
    DL_data_buffer_Cvector_List[125] = &DL_data_buffer_Cvector_125;
    DL_data_buffer_Cvector_List[126] = &DL_data_buffer_Cvector_126;
    DL_data_buffer_Cvector_List[127] = &DL_data_buffer_Cvector_127;
    DL_data_buffer_Cvector_List[128] = &DL_data_buffer_Cvector_128;
    DL_data_buffer_Cvector_List[129] = &DL_data_buffer_Cvector_129;
    DL_data_buffer_Cvector_List[130] = &DL_data_buffer_Cvector_130;
    DL_data_buffer_Cvector_List[131] = &DL_data_buffer_Cvector_131;
    DL_data_buffer_Cvector_List[132] = &DL_data_buffer_Cvector_132;
    DL_data_buffer_Cvector_List[133] = &DL_data_buffer_Cvector_133;
    DL_data_buffer_Cvector_List[134] = &DL_data_buffer_Cvector_134;
    DL_data_buffer_Cvector_List[135] = &DL_data_buffer_Cvector_135;
    DL_data_buffer_Cvector_List[136] = &DL_data_buffer_Cvector_136;
    DL_data_buffer_Cvector_List[137] = &DL_data_buffer_Cvector_137;
    DL_data_buffer_Cvector_List[138] = &DL_data_buffer_Cvector_138;
    DL_data_buffer_Cvector_List[139] = &DL_data_buffer_Cvector_139;
    DL_data_buffer_Cvector_List[140] = &DL_data_buffer_Cvector_140;
    DL_data_buffer_Cvector_List[141] = &DL_data_buffer_Cvector_141;
    DL_data_buffer_Cvector_List[142] = &DL_data_buffer_Cvector_142;
    DL_data_buffer_Cvector_List[143] = &DL_data_buffer_Cvector_143;
    DL_data_buffer_Cvector_List[144] = &DL_data_buffer_Cvector_144;
    DL_data_buffer_Cvector_List[145] = &DL_data_buffer_Cvector_145;
    DL_data_buffer_Cvector_List[146] = &DL_data_buffer_Cvector_146;
    DL_data_buffer_Cvector_List[147] = &DL_data_buffer_Cvector_147;
    DL_data_buffer_Cvector_List[148] = &DL_data_buffer_Cvector_148;
    DL_data_buffer_Cvector_List[149] = &DL_data_buffer_Cvector_149;
    DL_data_buffer_Cvector_List[150] = &DL_data_buffer_Cvector_150;
    DL_data_buffer_Cvector_List[151] = &DL_data_buffer_Cvector_151;
    DL_data_buffer_Cvector_List[152] = &DL_data_buffer_Cvector_152;
    DL_data_buffer_Cvector_List[153] = &DL_data_buffer_Cvector_153;
    DL_data_buffer_Cvector_List[154] = &DL_data_buffer_Cvector_154;
    DL_data_buffer_Cvector_List[155] = &DL_data_buffer_Cvector_155;
    DL_data_buffer_Cvector_List[156] = &DL_data_buffer_Cvector_156;
    DL_data_buffer_Cvector_List[157] = &DL_data_buffer_Cvector_157;
    DL_data_buffer_Cvector_List[158] = &DL_data_buffer_Cvector_158;
    DL_data_buffer_Cvector_List[159] = &DL_data_buffer_Cvector_159;
    DL_data_buffer_Cvector_List[160] = &DL_data_buffer_Cvector_160;
    DL_data_buffer_Cvector_List[161] = &DL_data_buffer_Cvector_161;
    DL_data_buffer_Cvector_List[162] = &DL_data_buffer_Cvector_162;
    DL_data_buffer_Cvector_List[163] = &DL_data_buffer_Cvector_163;
    DL_data_buffer_Cvector_List[164] = &DL_data_buffer_Cvector_164;
    DL_data_buffer_Cvector_List[165] = &DL_data_buffer_Cvector_165;
    DL_data_buffer_Cvector_List[166] = &DL_data_buffer_Cvector_166;
    DL_data_buffer_Cvector_List[167] = &DL_data_buffer_Cvector_167;
    DL_data_buffer_Cvector_List[168] = &DL_data_buffer_Cvector_168;
    DL_data_buffer_Cvector_List[169] = &DL_data_buffer_Cvector_169;
    DL_data_buffer_Cvector_List[170] = &DL_data_buffer_Cvector_170;
    DL_data_buffer_Cvector_List[171] = &DL_data_buffer_Cvector_171;
    DL_data_buffer_Cvector_List[172] = &DL_data_buffer_Cvector_172;
    DL_data_buffer_Cvector_List[173] = &DL_data_buffer_Cvector_173;
    DL_data_buffer_Cvector_List[174] = &DL_data_buffer_Cvector_174;
    DL_data_buffer_Cvector_List[175] = &DL_data_buffer_Cvector_175;
    DL_data_buffer_Cvector_List[176] = &DL_data_buffer_Cvector_176;
    DL_data_buffer_Cvector_List[177] = &DL_data_buffer_Cvector_177;
    DL_data_buffer_Cvector_List[178] = &DL_data_buffer_Cvector_178;
    DL_data_buffer_Cvector_List[179] = &DL_data_buffer_Cvector_179;
    DL_data_buffer_Cvector_List[180] = &DL_data_buffer_Cvector_180;
    DL_data_buffer_Cvector_List[181] = &DL_data_buffer_Cvector_181;
    DL_data_buffer_Cvector_List[182] = &DL_data_buffer_Cvector_182;
    DL_data_buffer_Cvector_List[183] = &DL_data_buffer_Cvector_183;
    DL_data_buffer_Cvector_List[184] = &DL_data_buffer_Cvector_184;
    DL_data_buffer_Cvector_List[185] = &DL_data_buffer_Cvector_185;
    DL_data_buffer_Cvector_List[186] = &DL_data_buffer_Cvector_186;
    DL_data_buffer_Cvector_List[187] = &DL_data_buffer_Cvector_187;
    DL_data_buffer_Cvector_List[188] = &DL_data_buffer_Cvector_188;
    DL_data_buffer_Cvector_List[189] = &DL_data_buffer_Cvector_189;
    DL_data_buffer_Cvector_List[190] = &DL_data_buffer_Cvector_190;
    DL_data_buffer_Cvector_List[191] = &DL_data_buffer_Cvector_191;
    DL_data_buffer_Cvector_List[192] = &DL_data_buffer_Cvector_192;
    DL_data_buffer_Cvector_List[193] = &DL_data_buffer_Cvector_193;
    DL_data_buffer_Cvector_List[194] = &DL_data_buffer_Cvector_194;
    DL_data_buffer_Cvector_List[195] = &DL_data_buffer_Cvector_195;
    DL_data_buffer_Cvector_List[196] = &DL_data_buffer_Cvector_196;
    DL_data_buffer_Cvector_List[197] = &DL_data_buffer_Cvector_197;
    DL_data_buffer_Cvector_List[198] = &DL_data_buffer_Cvector_198;
    DL_data_buffer_Cvector_List[199] = &DL_data_buffer_Cvector_199;
    DL_data_buffer_Cvector_List[200] = &DL_data_buffer_Cvector_200;
    DL_data_buffer_Cvector_List[201] = &DL_data_buffer_Cvector_201;
    DL_data_buffer_Cvector_List[202] = &DL_data_buffer_Cvector_202;
    DL_data_buffer_Cvector_List[203] = &DL_data_buffer_Cvector_203;
    DL_data_buffer_Cvector_List[204] = &DL_data_buffer_Cvector_204;
    DL_data_buffer_Cvector_List[205] = &DL_data_buffer_Cvector_205;
    DL_data_buffer_Cvector_List[206] = &DL_data_buffer_Cvector_206;
    DL_data_buffer_Cvector_List[207] = &DL_data_buffer_Cvector_207;
    DL_data_buffer_Cvector_List[208] = &DL_data_buffer_Cvector_208;
    DL_data_buffer_Cvector_List[209] = &DL_data_buffer_Cvector_209;
    DL_data_buffer_Cvector_List[210] = &DL_data_buffer_Cvector_210;
    DL_data_buffer_Cvector_List[211] = &DL_data_buffer_Cvector_211;
    DL_data_buffer_Cvector_List[212] = &DL_data_buffer_Cvector_212;
    DL_data_buffer_Cvector_List[213] = &DL_data_buffer_Cvector_213;
    DL_data_buffer_Cvector_List[214] = &DL_data_buffer_Cvector_214;
    DL_data_buffer_Cvector_List[215] = &DL_data_buffer_Cvector_215;
    DL_data_buffer_Cvector_List[216] = &DL_data_buffer_Cvector_216;
    DL_data_buffer_Cvector_List[217] = &DL_data_buffer_Cvector_217;
    DL_data_buffer_Cvector_List[218] = &DL_data_buffer_Cvector_218;
    DL_data_buffer_Cvector_List[219] = &DL_data_buffer_Cvector_219;
    DL_data_buffer_Cvector_List[220] = &DL_data_buffer_Cvector_220;
    DL_data_buffer_Cvector_List[221] = &DL_data_buffer_Cvector_221;
    DL_data_buffer_Cvector_List[222] = &DL_data_buffer_Cvector_222;
    DL_data_buffer_Cvector_List[223] = &DL_data_buffer_Cvector_223;
    DL_data_buffer_Cvector_List[224] = &DL_data_buffer_Cvector_224;
    DL_data_buffer_Cvector_List[225] = &DL_data_buffer_Cvector_225;
    DL_data_buffer_Cvector_List[226] = &DL_data_buffer_Cvector_226;
    DL_data_buffer_Cvector_List[227] = &DL_data_buffer_Cvector_227;
    DL_data_buffer_Cvector_List[228] = &DL_data_buffer_Cvector_228;
    DL_data_buffer_Cvector_List[229] = &DL_data_buffer_Cvector_229;
    DL_data_buffer_Cvector_List[230] = &DL_data_buffer_Cvector_230;
    DL_data_buffer_Cvector_List[231] = &DL_data_buffer_Cvector_231;
    DL_data_buffer_Cvector_List[232] = &DL_data_buffer_Cvector_232;
    DL_data_buffer_Cvector_List[233] = &DL_data_buffer_Cvector_233;
    DL_data_buffer_Cvector_List[234] = &DL_data_buffer_Cvector_234;
    DL_data_buffer_Cvector_List[235] = &DL_data_buffer_Cvector_235;
    DL_data_buffer_Cvector_List[236] = &DL_data_buffer_Cvector_236;
    DL_data_buffer_Cvector_List[237] = &DL_data_buffer_Cvector_237;
    DL_data_buffer_Cvector_List[238] = &DL_data_buffer_Cvector_238;
    DL_data_buffer_Cvector_List[239] = &DL_data_buffer_Cvector_239;
    DL_data_buffer_Cvector_List[240] = &DL_data_buffer_Cvector_240;
    DL_data_buffer_Cvector_List[241] = &DL_data_buffer_Cvector_241;
    DL_data_buffer_Cvector_List[242] = &DL_data_buffer_Cvector_242;
    DL_data_buffer_Cvector_List[243] = &DL_data_buffer_Cvector_243;
    DL_data_buffer_Cvector_List[244] = &DL_data_buffer_Cvector_244;
    DL_data_buffer_Cvector_List[245] = &DL_data_buffer_Cvector_245;
    DL_data_buffer_Cvector_List[246] = &DL_data_buffer_Cvector_246;
    DL_data_buffer_Cvector_List[247] = &DL_data_buffer_Cvector_247;
    DL_data_buffer_Cvector_List[248] = &DL_data_buffer_Cvector_248;
    DL_data_buffer_Cvector_List[249] = &DL_data_buffer_Cvector_249;

    int index;
    index = 0;
    for (index; index < BBU_UE_MAXNUM; index++)
    {
        cvector_init(DL_data_buffer_Cvector_List[index]);
    }
}

/*皇甫阳阳*/
void DL_data_buffer_Queue_List_init(App_queue **DL_data_buffer_Queue_List)
{
    DL_data_buffer_Queue_List[0] = &Queue_DL_DATA_Buffer_000;
    DL_data_buffer_Queue_List[1] = &Queue_DL_DATA_Buffer_001;
    DL_data_buffer_Queue_List[2] = &Queue_DL_DATA_Buffer_002;
    DL_data_buffer_Queue_List[3] = &Queue_DL_DATA_Buffer_003;
    DL_data_buffer_Queue_List[4] = &Queue_DL_DATA_Buffer_004;
    DL_data_buffer_Queue_List[5] = &Queue_DL_DATA_Buffer_005;
    DL_data_buffer_Queue_List[6] = &Queue_DL_DATA_Buffer_006;
    DL_data_buffer_Queue_List[7] = &Queue_DL_DATA_Buffer_007;
    DL_data_buffer_Queue_List[8] = &Queue_DL_DATA_Buffer_008;
    DL_data_buffer_Queue_List[9] = &Queue_DL_DATA_Buffer_009;
    DL_data_buffer_Queue_List[10] = &Queue_DL_DATA_Buffer_010;
    DL_data_buffer_Queue_List[11] = &Queue_DL_DATA_Buffer_011;
    DL_data_buffer_Queue_List[12] = &Queue_DL_DATA_Buffer_012;
    DL_data_buffer_Queue_List[13] = &Queue_DL_DATA_Buffer_013;
    DL_data_buffer_Queue_List[14] = &Queue_DL_DATA_Buffer_014;
    DL_data_buffer_Queue_List[15] = &Queue_DL_DATA_Buffer_015;
    DL_data_buffer_Queue_List[16] = &Queue_DL_DATA_Buffer_016;
    DL_data_buffer_Queue_List[17] = &Queue_DL_DATA_Buffer_017;
    DL_data_buffer_Queue_List[18] = &Queue_DL_DATA_Buffer_018;
    DL_data_buffer_Queue_List[19] = &Queue_DL_DATA_Buffer_019;
    DL_data_buffer_Queue_List[20] = &Queue_DL_DATA_Buffer_020;
    DL_data_buffer_Queue_List[21] = &Queue_DL_DATA_Buffer_021;
    DL_data_buffer_Queue_List[22] = &Queue_DL_DATA_Buffer_022;
    DL_data_buffer_Queue_List[23] = &Queue_DL_DATA_Buffer_023;
    DL_data_buffer_Queue_List[24] = &Queue_DL_DATA_Buffer_024;
    DL_data_buffer_Queue_List[25] = &Queue_DL_DATA_Buffer_025;
    DL_data_buffer_Queue_List[26] = &Queue_DL_DATA_Buffer_026;
    DL_data_buffer_Queue_List[27] = &Queue_DL_DATA_Buffer_027;
    DL_data_buffer_Queue_List[28] = &Queue_DL_DATA_Buffer_028;
    DL_data_buffer_Queue_List[29] = &Queue_DL_DATA_Buffer_029;
    DL_data_buffer_Queue_List[30] = &Queue_DL_DATA_Buffer_030;
    DL_data_buffer_Queue_List[31] = &Queue_DL_DATA_Buffer_031;
    DL_data_buffer_Queue_List[32] = &Queue_DL_DATA_Buffer_032;
    DL_data_buffer_Queue_List[33] = &Queue_DL_DATA_Buffer_033;
    DL_data_buffer_Queue_List[34] = &Queue_DL_DATA_Buffer_034;
    DL_data_buffer_Queue_List[35] = &Queue_DL_DATA_Buffer_035;
    DL_data_buffer_Queue_List[36] = &Queue_DL_DATA_Buffer_036;
    DL_data_buffer_Queue_List[37] = &Queue_DL_DATA_Buffer_037;
    DL_data_buffer_Queue_List[38] = &Queue_DL_DATA_Buffer_038;
    DL_data_buffer_Queue_List[39] = &Queue_DL_DATA_Buffer_039;
    DL_data_buffer_Queue_List[40] = &Queue_DL_DATA_Buffer_040;
    DL_data_buffer_Queue_List[41] = &Queue_DL_DATA_Buffer_041;
    DL_data_buffer_Queue_List[42] = &Queue_DL_DATA_Buffer_042;
    DL_data_buffer_Queue_List[43] = &Queue_DL_DATA_Buffer_043;
    DL_data_buffer_Queue_List[44] = &Queue_DL_DATA_Buffer_044;
    DL_data_buffer_Queue_List[45] = &Queue_DL_DATA_Buffer_045;
    DL_data_buffer_Queue_List[46] = &Queue_DL_DATA_Buffer_046;
    DL_data_buffer_Queue_List[47] = &Queue_DL_DATA_Buffer_047;
    DL_data_buffer_Queue_List[48] = &Queue_DL_DATA_Buffer_048;
    DL_data_buffer_Queue_List[49] = &Queue_DL_DATA_Buffer_049;
    DL_data_buffer_Queue_List[50] = &Queue_DL_DATA_Buffer_050;
    DL_data_buffer_Queue_List[51] = &Queue_DL_DATA_Buffer_051;
    DL_data_buffer_Queue_List[52] = &Queue_DL_DATA_Buffer_052;
    DL_data_buffer_Queue_List[53] = &Queue_DL_DATA_Buffer_053;
    DL_data_buffer_Queue_List[54] = &Queue_DL_DATA_Buffer_054;
    DL_data_buffer_Queue_List[55] = &Queue_DL_DATA_Buffer_055;
    DL_data_buffer_Queue_List[56] = &Queue_DL_DATA_Buffer_056;
    DL_data_buffer_Queue_List[57] = &Queue_DL_DATA_Buffer_057;
    DL_data_buffer_Queue_List[58] = &Queue_DL_DATA_Buffer_058;
    DL_data_buffer_Queue_List[59] = &Queue_DL_DATA_Buffer_059;
    DL_data_buffer_Queue_List[60] = &Queue_DL_DATA_Buffer_060;
    DL_data_buffer_Queue_List[61] = &Queue_DL_DATA_Buffer_061;
    DL_data_buffer_Queue_List[62] = &Queue_DL_DATA_Buffer_062;
    DL_data_buffer_Queue_List[63] = &Queue_DL_DATA_Buffer_063;
    DL_data_buffer_Queue_List[64] = &Queue_DL_DATA_Buffer_064;
    DL_data_buffer_Queue_List[65] = &Queue_DL_DATA_Buffer_065;
    DL_data_buffer_Queue_List[66] = &Queue_DL_DATA_Buffer_066;
    DL_data_buffer_Queue_List[67] = &Queue_DL_DATA_Buffer_067;
    DL_data_buffer_Queue_List[68] = &Queue_DL_DATA_Buffer_068;
    DL_data_buffer_Queue_List[69] = &Queue_DL_DATA_Buffer_069;
    DL_data_buffer_Queue_List[70] = &Queue_DL_DATA_Buffer_070;
    DL_data_buffer_Queue_List[71] = &Queue_DL_DATA_Buffer_071;
    DL_data_buffer_Queue_List[72] = &Queue_DL_DATA_Buffer_072;
    DL_data_buffer_Queue_List[73] = &Queue_DL_DATA_Buffer_073;
    DL_data_buffer_Queue_List[74] = &Queue_DL_DATA_Buffer_074;
    DL_data_buffer_Queue_List[75] = &Queue_DL_DATA_Buffer_075;
    DL_data_buffer_Queue_List[76] = &Queue_DL_DATA_Buffer_076;
    DL_data_buffer_Queue_List[77] = &Queue_DL_DATA_Buffer_077;
    DL_data_buffer_Queue_List[78] = &Queue_DL_DATA_Buffer_078;
    DL_data_buffer_Queue_List[79] = &Queue_DL_DATA_Buffer_079;
    DL_data_buffer_Queue_List[80] = &Queue_DL_DATA_Buffer_080;
    DL_data_buffer_Queue_List[81] = &Queue_DL_DATA_Buffer_081;
    DL_data_buffer_Queue_List[82] = &Queue_DL_DATA_Buffer_082;
    DL_data_buffer_Queue_List[83] = &Queue_DL_DATA_Buffer_083;
    DL_data_buffer_Queue_List[84] = &Queue_DL_DATA_Buffer_084;
    DL_data_buffer_Queue_List[85] = &Queue_DL_DATA_Buffer_085;
    DL_data_buffer_Queue_List[86] = &Queue_DL_DATA_Buffer_086;
    DL_data_buffer_Queue_List[87] = &Queue_DL_DATA_Buffer_087;
    DL_data_buffer_Queue_List[88] = &Queue_DL_DATA_Buffer_088;
    DL_data_buffer_Queue_List[89] = &Queue_DL_DATA_Buffer_089;
    DL_data_buffer_Queue_List[90] = &Queue_DL_DATA_Buffer_090;
    DL_data_buffer_Queue_List[91] = &Queue_DL_DATA_Buffer_091;
    DL_data_buffer_Queue_List[92] = &Queue_DL_DATA_Buffer_092;
    DL_data_buffer_Queue_List[93] = &Queue_DL_DATA_Buffer_093;
    DL_data_buffer_Queue_List[94] = &Queue_DL_DATA_Buffer_094;
    DL_data_buffer_Queue_List[95] = &Queue_DL_DATA_Buffer_095;
    DL_data_buffer_Queue_List[96] = &Queue_DL_DATA_Buffer_096;
    DL_data_buffer_Queue_List[97] = &Queue_DL_DATA_Buffer_097;
    DL_data_buffer_Queue_List[98] = &Queue_DL_DATA_Buffer_098;
    DL_data_buffer_Queue_List[99] = &Queue_DL_DATA_Buffer_099;
    DL_data_buffer_Queue_List[100] = &Queue_DL_DATA_Buffer_100;
    DL_data_buffer_Queue_List[101] = &Queue_DL_DATA_Buffer_101;
    DL_data_buffer_Queue_List[102] = &Queue_DL_DATA_Buffer_102;
    DL_data_buffer_Queue_List[103] = &Queue_DL_DATA_Buffer_103;
    DL_data_buffer_Queue_List[104] = &Queue_DL_DATA_Buffer_104;
    DL_data_buffer_Queue_List[105] = &Queue_DL_DATA_Buffer_105;
    DL_data_buffer_Queue_List[106] = &Queue_DL_DATA_Buffer_106;
    DL_data_buffer_Queue_List[107] = &Queue_DL_DATA_Buffer_107;
    DL_data_buffer_Queue_List[108] = &Queue_DL_DATA_Buffer_108;
    DL_data_buffer_Queue_List[109] = &Queue_DL_DATA_Buffer_109;
    DL_data_buffer_Queue_List[110] = &Queue_DL_DATA_Buffer_110;
    DL_data_buffer_Queue_List[111] = &Queue_DL_DATA_Buffer_111;
    DL_data_buffer_Queue_List[112] = &Queue_DL_DATA_Buffer_112;
    DL_data_buffer_Queue_List[113] = &Queue_DL_DATA_Buffer_113;
    DL_data_buffer_Queue_List[114] = &Queue_DL_DATA_Buffer_114;
    DL_data_buffer_Queue_List[115] = &Queue_DL_DATA_Buffer_115;
    DL_data_buffer_Queue_List[116] = &Queue_DL_DATA_Buffer_116;
    DL_data_buffer_Queue_List[117] = &Queue_DL_DATA_Buffer_117;
    DL_data_buffer_Queue_List[118] = &Queue_DL_DATA_Buffer_118;
    DL_data_buffer_Queue_List[119] = &Queue_DL_DATA_Buffer_119;
    DL_data_buffer_Queue_List[120] = &Queue_DL_DATA_Buffer_120;
    DL_data_buffer_Queue_List[121] = &Queue_DL_DATA_Buffer_121;
    DL_data_buffer_Queue_List[122] = &Queue_DL_DATA_Buffer_122;
    DL_data_buffer_Queue_List[123] = &Queue_DL_DATA_Buffer_123;
    DL_data_buffer_Queue_List[124] = &Queue_DL_DATA_Buffer_124;
    DL_data_buffer_Queue_List[125] = &Queue_DL_DATA_Buffer_125;
    DL_data_buffer_Queue_List[126] = &Queue_DL_DATA_Buffer_126;
    DL_data_buffer_Queue_List[127] = &Queue_DL_DATA_Buffer_127;
    DL_data_buffer_Queue_List[128] = &Queue_DL_DATA_Buffer_128;
    DL_data_buffer_Queue_List[129] = &Queue_DL_DATA_Buffer_129;
    DL_data_buffer_Queue_List[130] = &Queue_DL_DATA_Buffer_130;
    DL_data_buffer_Queue_List[131] = &Queue_DL_DATA_Buffer_131;
    DL_data_buffer_Queue_List[132] = &Queue_DL_DATA_Buffer_132;
    DL_data_buffer_Queue_List[133] = &Queue_DL_DATA_Buffer_133;
    DL_data_buffer_Queue_List[134] = &Queue_DL_DATA_Buffer_134;
    DL_data_buffer_Queue_List[135] = &Queue_DL_DATA_Buffer_135;
    DL_data_buffer_Queue_List[136] = &Queue_DL_DATA_Buffer_136;
    DL_data_buffer_Queue_List[137] = &Queue_DL_DATA_Buffer_137;
    DL_data_buffer_Queue_List[138] = &Queue_DL_DATA_Buffer_138;
    DL_data_buffer_Queue_List[139] = &Queue_DL_DATA_Buffer_139;
    DL_data_buffer_Queue_List[140] = &Queue_DL_DATA_Buffer_140;
    DL_data_buffer_Queue_List[141] = &Queue_DL_DATA_Buffer_141;
    DL_data_buffer_Queue_List[142] = &Queue_DL_DATA_Buffer_142;
    DL_data_buffer_Queue_List[143] = &Queue_DL_DATA_Buffer_143;
    DL_data_buffer_Queue_List[144] = &Queue_DL_DATA_Buffer_144;
    DL_data_buffer_Queue_List[145] = &Queue_DL_DATA_Buffer_145;
    DL_data_buffer_Queue_List[146] = &Queue_DL_DATA_Buffer_146;
    DL_data_buffer_Queue_List[147] = &Queue_DL_DATA_Buffer_147;
    DL_data_buffer_Queue_List[148] = &Queue_DL_DATA_Buffer_148;
    DL_data_buffer_Queue_List[149] = &Queue_DL_DATA_Buffer_149;
    DL_data_buffer_Queue_List[150] = &Queue_DL_DATA_Buffer_150;
    DL_data_buffer_Queue_List[151] = &Queue_DL_DATA_Buffer_151;
    DL_data_buffer_Queue_List[152] = &Queue_DL_DATA_Buffer_152;
    DL_data_buffer_Queue_List[153] = &Queue_DL_DATA_Buffer_153;
    DL_data_buffer_Queue_List[154] = &Queue_DL_DATA_Buffer_154;
    DL_data_buffer_Queue_List[155] = &Queue_DL_DATA_Buffer_155;
    DL_data_buffer_Queue_List[156] = &Queue_DL_DATA_Buffer_156;
    DL_data_buffer_Queue_List[157] = &Queue_DL_DATA_Buffer_157;
    DL_data_buffer_Queue_List[158] = &Queue_DL_DATA_Buffer_158;
    DL_data_buffer_Queue_List[159] = &Queue_DL_DATA_Buffer_159;
    DL_data_buffer_Queue_List[160] = &Queue_DL_DATA_Buffer_160;
    DL_data_buffer_Queue_List[161] = &Queue_DL_DATA_Buffer_161;
    DL_data_buffer_Queue_List[162] = &Queue_DL_DATA_Buffer_162;
    DL_data_buffer_Queue_List[163] = &Queue_DL_DATA_Buffer_163;
    DL_data_buffer_Queue_List[164] = &Queue_DL_DATA_Buffer_164;
    DL_data_buffer_Queue_List[165] = &Queue_DL_DATA_Buffer_165;
    DL_data_buffer_Queue_List[166] = &Queue_DL_DATA_Buffer_166;
    DL_data_buffer_Queue_List[167] = &Queue_DL_DATA_Buffer_167;
    DL_data_buffer_Queue_List[168] = &Queue_DL_DATA_Buffer_168;
    DL_data_buffer_Queue_List[169] = &Queue_DL_DATA_Buffer_169;
    DL_data_buffer_Queue_List[170] = &Queue_DL_DATA_Buffer_170;
    DL_data_buffer_Queue_List[171] = &Queue_DL_DATA_Buffer_171;
    DL_data_buffer_Queue_List[172] = &Queue_DL_DATA_Buffer_172;
    DL_data_buffer_Queue_List[173] = &Queue_DL_DATA_Buffer_173;
    DL_data_buffer_Queue_List[174] = &Queue_DL_DATA_Buffer_174;
    DL_data_buffer_Queue_List[175] = &Queue_DL_DATA_Buffer_175;
    DL_data_buffer_Queue_List[176] = &Queue_DL_DATA_Buffer_176;
    DL_data_buffer_Queue_List[177] = &Queue_DL_DATA_Buffer_177;
    DL_data_buffer_Queue_List[178] = &Queue_DL_DATA_Buffer_178;
    DL_data_buffer_Queue_List[179] = &Queue_DL_DATA_Buffer_179;
    DL_data_buffer_Queue_List[180] = &Queue_DL_DATA_Buffer_180;
    DL_data_buffer_Queue_List[181] = &Queue_DL_DATA_Buffer_181;
    DL_data_buffer_Queue_List[182] = &Queue_DL_DATA_Buffer_182;
    DL_data_buffer_Queue_List[183] = &Queue_DL_DATA_Buffer_183;
    DL_data_buffer_Queue_List[184] = &Queue_DL_DATA_Buffer_184;
    DL_data_buffer_Queue_List[185] = &Queue_DL_DATA_Buffer_185;
    DL_data_buffer_Queue_List[186] = &Queue_DL_DATA_Buffer_186;
    DL_data_buffer_Queue_List[187] = &Queue_DL_DATA_Buffer_187;
    DL_data_buffer_Queue_List[188] = &Queue_DL_DATA_Buffer_188;
    DL_data_buffer_Queue_List[189] = &Queue_DL_DATA_Buffer_189;
    DL_data_buffer_Queue_List[190] = &Queue_DL_DATA_Buffer_190;
    DL_data_buffer_Queue_List[191] = &Queue_DL_DATA_Buffer_191;
    DL_data_buffer_Queue_List[192] = &Queue_DL_DATA_Buffer_192;
    DL_data_buffer_Queue_List[193] = &Queue_DL_DATA_Buffer_193;
    DL_data_buffer_Queue_List[194] = &Queue_DL_DATA_Buffer_194;
    DL_data_buffer_Queue_List[195] = &Queue_DL_DATA_Buffer_195;
    DL_data_buffer_Queue_List[196] = &Queue_DL_DATA_Buffer_196;
    DL_data_buffer_Queue_List[197] = &Queue_DL_DATA_Buffer_197;
    DL_data_buffer_Queue_List[198] = &Queue_DL_DATA_Buffer_198;
    DL_data_buffer_Queue_List[199] = &Queue_DL_DATA_Buffer_199;
    DL_data_buffer_Queue_List[200] = &Queue_DL_DATA_Buffer_200;
    DL_data_buffer_Queue_List[201] = &Queue_DL_DATA_Buffer_201;
    DL_data_buffer_Queue_List[202] = &Queue_DL_DATA_Buffer_202;
    DL_data_buffer_Queue_List[203] = &Queue_DL_DATA_Buffer_203;
    DL_data_buffer_Queue_List[204] = &Queue_DL_DATA_Buffer_204;
    DL_data_buffer_Queue_List[205] = &Queue_DL_DATA_Buffer_205;
    DL_data_buffer_Queue_List[206] = &Queue_DL_DATA_Buffer_206;
    DL_data_buffer_Queue_List[207] = &Queue_DL_DATA_Buffer_207;
    DL_data_buffer_Queue_List[208] = &Queue_DL_DATA_Buffer_208;
    DL_data_buffer_Queue_List[209] = &Queue_DL_DATA_Buffer_209;
    DL_data_buffer_Queue_List[210] = &Queue_DL_DATA_Buffer_210;
    DL_data_buffer_Queue_List[211] = &Queue_DL_DATA_Buffer_211;
    DL_data_buffer_Queue_List[212] = &Queue_DL_DATA_Buffer_212;
    DL_data_buffer_Queue_List[213] = &Queue_DL_DATA_Buffer_213;
    DL_data_buffer_Queue_List[214] = &Queue_DL_DATA_Buffer_214;
    DL_data_buffer_Queue_List[215] = &Queue_DL_DATA_Buffer_215;
    DL_data_buffer_Queue_List[216] = &Queue_DL_DATA_Buffer_216;
    DL_data_buffer_Queue_List[217] = &Queue_DL_DATA_Buffer_217;
    DL_data_buffer_Queue_List[218] = &Queue_DL_DATA_Buffer_218;
    DL_data_buffer_Queue_List[219] = &Queue_DL_DATA_Buffer_219;
    DL_data_buffer_Queue_List[220] = &Queue_DL_DATA_Buffer_220;
    DL_data_buffer_Queue_List[221] = &Queue_DL_DATA_Buffer_221;
    DL_data_buffer_Queue_List[222] = &Queue_DL_DATA_Buffer_222;
    DL_data_buffer_Queue_List[223] = &Queue_DL_DATA_Buffer_223;
    DL_data_buffer_Queue_List[224] = &Queue_DL_DATA_Buffer_224;
    DL_data_buffer_Queue_List[225] = &Queue_DL_DATA_Buffer_225;
    DL_data_buffer_Queue_List[226] = &Queue_DL_DATA_Buffer_226;
    DL_data_buffer_Queue_List[227] = &Queue_DL_DATA_Buffer_227;
    DL_data_buffer_Queue_List[228] = &Queue_DL_DATA_Buffer_228;
    DL_data_buffer_Queue_List[229] = &Queue_DL_DATA_Buffer_229;
    DL_data_buffer_Queue_List[230] = &Queue_DL_DATA_Buffer_230;
    DL_data_buffer_Queue_List[231] = &Queue_DL_DATA_Buffer_231;
    DL_data_buffer_Queue_List[232] = &Queue_DL_DATA_Buffer_232;
    DL_data_buffer_Queue_List[233] = &Queue_DL_DATA_Buffer_233;
    DL_data_buffer_Queue_List[234] = &Queue_DL_DATA_Buffer_234;
    DL_data_buffer_Queue_List[235] = &Queue_DL_DATA_Buffer_235;
    DL_data_buffer_Queue_List[236] = &Queue_DL_DATA_Buffer_236;
    DL_data_buffer_Queue_List[237] = &Queue_DL_DATA_Buffer_237;
    DL_data_buffer_Queue_List[238] = &Queue_DL_DATA_Buffer_238;
    DL_data_buffer_Queue_List[239] = &Queue_DL_DATA_Buffer_239;
    DL_data_buffer_Queue_List[240] = &Queue_DL_DATA_Buffer_240;
    DL_data_buffer_Queue_List[241] = &Queue_DL_DATA_Buffer_241;
    DL_data_buffer_Queue_List[242] = &Queue_DL_DATA_Buffer_242;
    DL_data_buffer_Queue_List[243] = &Queue_DL_DATA_Buffer_243;
    DL_data_buffer_Queue_List[244] = &Queue_DL_DATA_Buffer_244;
    DL_data_buffer_Queue_List[245] = &Queue_DL_DATA_Buffer_245;
    DL_data_buffer_Queue_List[246] = &Queue_DL_DATA_Buffer_246;
    DL_data_buffer_Queue_List[247] = &Queue_DL_DATA_Buffer_247;
    DL_data_buffer_Queue_List[248] = &Queue_DL_DATA_Buffer_248;
    DL_data_buffer_Queue_List[249] = &Queue_DL_DATA_Buffer_249;

    u8 queue_index = 0;
    for (queue_index; queue_index < BBU_UE_MAXNUM; queue_index++)
    {
        App_queue_init(DL_data_buffer_Queue_List[queue_index]);
    }
}

void UE_Data_Recv_Queue_List_init(App_queue **UE_data_recv_Queue_List)
{
    UE_data_recv_Queue_List[0] = &Queue_UE_data_recv_000;
    UE_data_recv_Queue_List[1] = &Queue_UE_data_recv_001;
    UE_data_recv_Queue_List[2] = &Queue_UE_data_recv_002;
    UE_data_recv_Queue_List[3] = &Queue_UE_data_recv_003;
    UE_data_recv_Queue_List[4] = &Queue_UE_data_recv_004;
    UE_data_recv_Queue_List[5] = &Queue_UE_data_recv_005;
    UE_data_recv_Queue_List[6] = &Queue_UE_data_recv_006;
    UE_data_recv_Queue_List[7] = &Queue_UE_data_recv_007;
    UE_data_recv_Queue_List[8] = &Queue_UE_data_recv_008;
    UE_data_recv_Queue_List[9] = &Queue_UE_data_recv_009;
    UE_data_recv_Queue_List[10] = &Queue_UE_data_recv_010;
    UE_data_recv_Queue_List[11] = &Queue_UE_data_recv_011;
    UE_data_recv_Queue_List[12] = &Queue_UE_data_recv_012;
    UE_data_recv_Queue_List[13] = &Queue_UE_data_recv_013;
    UE_data_recv_Queue_List[14] = &Queue_UE_data_recv_014;
    UE_data_recv_Queue_List[15] = &Queue_UE_data_recv_015;
    UE_data_recv_Queue_List[16] = &Queue_UE_data_recv_016;
    UE_data_recv_Queue_List[17] = &Queue_UE_data_recv_017;
    UE_data_recv_Queue_List[18] = &Queue_UE_data_recv_018;
    UE_data_recv_Queue_List[19] = &Queue_UE_data_recv_019;
    UE_data_recv_Queue_List[20] = &Queue_UE_data_recv_020;
    UE_data_recv_Queue_List[21] = &Queue_UE_data_recv_021;
    UE_data_recv_Queue_List[22] = &Queue_UE_data_recv_022;
    UE_data_recv_Queue_List[23] = &Queue_UE_data_recv_023;
    UE_data_recv_Queue_List[24] = &Queue_UE_data_recv_024;
    UE_data_recv_Queue_List[25] = &Queue_UE_data_recv_025;
    UE_data_recv_Queue_List[26] = &Queue_UE_data_recv_026;
    UE_data_recv_Queue_List[27] = &Queue_UE_data_recv_027;
    UE_data_recv_Queue_List[28] = &Queue_UE_data_recv_028;
    UE_data_recv_Queue_List[29] = &Queue_UE_data_recv_029;
    UE_data_recv_Queue_List[30] = &Queue_UE_data_recv_030;
    UE_data_recv_Queue_List[31] = &Queue_UE_data_recv_031;
    UE_data_recv_Queue_List[32] = &Queue_UE_data_recv_032;
    UE_data_recv_Queue_List[33] = &Queue_UE_data_recv_033;
    UE_data_recv_Queue_List[34] = &Queue_UE_data_recv_034;
    UE_data_recv_Queue_List[35] = &Queue_UE_data_recv_035;
    UE_data_recv_Queue_List[36] = &Queue_UE_data_recv_036;
    UE_data_recv_Queue_List[37] = &Queue_UE_data_recv_037;
    UE_data_recv_Queue_List[38] = &Queue_UE_data_recv_038;
    UE_data_recv_Queue_List[39] = &Queue_UE_data_recv_039;
    UE_data_recv_Queue_List[40] = &Queue_UE_data_recv_040;
    UE_data_recv_Queue_List[41] = &Queue_UE_data_recv_041;
    UE_data_recv_Queue_List[42] = &Queue_UE_data_recv_042;
    UE_data_recv_Queue_List[43] = &Queue_UE_data_recv_043;
    UE_data_recv_Queue_List[44] = &Queue_UE_data_recv_044;
    UE_data_recv_Queue_List[45] = &Queue_UE_data_recv_045;
    UE_data_recv_Queue_List[46] = &Queue_UE_data_recv_046;
    UE_data_recv_Queue_List[47] = &Queue_UE_data_recv_047;
    UE_data_recv_Queue_List[48] = &Queue_UE_data_recv_048;
    UE_data_recv_Queue_List[49] = &Queue_UE_data_recv_049;
    UE_data_recv_Queue_List[50] = &Queue_UE_data_recv_050;
    UE_data_recv_Queue_List[51] = &Queue_UE_data_recv_051;
    UE_data_recv_Queue_List[52] = &Queue_UE_data_recv_052;
    UE_data_recv_Queue_List[53] = &Queue_UE_data_recv_053;
    UE_data_recv_Queue_List[54] = &Queue_UE_data_recv_054;
    UE_data_recv_Queue_List[55] = &Queue_UE_data_recv_055;
    UE_data_recv_Queue_List[56] = &Queue_UE_data_recv_056;
    UE_data_recv_Queue_List[57] = &Queue_UE_data_recv_057;
    UE_data_recv_Queue_List[58] = &Queue_UE_data_recv_058;
    UE_data_recv_Queue_List[59] = &Queue_UE_data_recv_059;
    UE_data_recv_Queue_List[60] = &Queue_UE_data_recv_060;
    UE_data_recv_Queue_List[61] = &Queue_UE_data_recv_061;
    UE_data_recv_Queue_List[62] = &Queue_UE_data_recv_062;
    UE_data_recv_Queue_List[63] = &Queue_UE_data_recv_063;
    UE_data_recv_Queue_List[64] = &Queue_UE_data_recv_064;
    UE_data_recv_Queue_List[65] = &Queue_UE_data_recv_065;
    UE_data_recv_Queue_List[66] = &Queue_UE_data_recv_066;
    UE_data_recv_Queue_List[67] = &Queue_UE_data_recv_067;
    UE_data_recv_Queue_List[68] = &Queue_UE_data_recv_068;
    UE_data_recv_Queue_List[69] = &Queue_UE_data_recv_069;
    UE_data_recv_Queue_List[70] = &Queue_UE_data_recv_070;
    UE_data_recv_Queue_List[71] = &Queue_UE_data_recv_071;
    UE_data_recv_Queue_List[72] = &Queue_UE_data_recv_072;
    UE_data_recv_Queue_List[73] = &Queue_UE_data_recv_073;
    UE_data_recv_Queue_List[74] = &Queue_UE_data_recv_074;
    UE_data_recv_Queue_List[75] = &Queue_UE_data_recv_075;
    UE_data_recv_Queue_List[76] = &Queue_UE_data_recv_076;
    UE_data_recv_Queue_List[77] = &Queue_UE_data_recv_077;
    UE_data_recv_Queue_List[78] = &Queue_UE_data_recv_078;
    UE_data_recv_Queue_List[79] = &Queue_UE_data_recv_079;
    UE_data_recv_Queue_List[80] = &Queue_UE_data_recv_080;
    UE_data_recv_Queue_List[81] = &Queue_UE_data_recv_081;
    UE_data_recv_Queue_List[82] = &Queue_UE_data_recv_082;
    UE_data_recv_Queue_List[83] = &Queue_UE_data_recv_083;
    UE_data_recv_Queue_List[84] = &Queue_UE_data_recv_084;
    UE_data_recv_Queue_List[85] = &Queue_UE_data_recv_085;
    UE_data_recv_Queue_List[86] = &Queue_UE_data_recv_086;
    UE_data_recv_Queue_List[87] = &Queue_UE_data_recv_087;
    UE_data_recv_Queue_List[88] = &Queue_UE_data_recv_088;
    UE_data_recv_Queue_List[89] = &Queue_UE_data_recv_089;
    UE_data_recv_Queue_List[90] = &Queue_UE_data_recv_090;
    UE_data_recv_Queue_List[91] = &Queue_UE_data_recv_091;
    UE_data_recv_Queue_List[92] = &Queue_UE_data_recv_092;
    UE_data_recv_Queue_List[93] = &Queue_UE_data_recv_093;
    UE_data_recv_Queue_List[94] = &Queue_UE_data_recv_094;
    UE_data_recv_Queue_List[95] = &Queue_UE_data_recv_095;
    UE_data_recv_Queue_List[96] = &Queue_UE_data_recv_096;
    UE_data_recv_Queue_List[97] = &Queue_UE_data_recv_097;
    UE_data_recv_Queue_List[98] = &Queue_UE_data_recv_098;
    UE_data_recv_Queue_List[99] = &Queue_UE_data_recv_099;
    UE_data_recv_Queue_List[100] = &Queue_UE_data_recv_100;
    UE_data_recv_Queue_List[101] = &Queue_UE_data_recv_101;
    UE_data_recv_Queue_List[102] = &Queue_UE_data_recv_102;
    UE_data_recv_Queue_List[103] = &Queue_UE_data_recv_103;
    UE_data_recv_Queue_List[104] = &Queue_UE_data_recv_104;
    UE_data_recv_Queue_List[105] = &Queue_UE_data_recv_105;
    UE_data_recv_Queue_List[106] = &Queue_UE_data_recv_106;
    UE_data_recv_Queue_List[107] = &Queue_UE_data_recv_107;
    UE_data_recv_Queue_List[108] = &Queue_UE_data_recv_108;
    UE_data_recv_Queue_List[109] = &Queue_UE_data_recv_109;
    UE_data_recv_Queue_List[110] = &Queue_UE_data_recv_110;
    UE_data_recv_Queue_List[111] = &Queue_UE_data_recv_111;
    UE_data_recv_Queue_List[112] = &Queue_UE_data_recv_112;
    UE_data_recv_Queue_List[113] = &Queue_UE_data_recv_113;
    UE_data_recv_Queue_List[114] = &Queue_UE_data_recv_114;
    UE_data_recv_Queue_List[115] = &Queue_UE_data_recv_115;
    UE_data_recv_Queue_List[116] = &Queue_UE_data_recv_116;
    UE_data_recv_Queue_List[117] = &Queue_UE_data_recv_117;
    UE_data_recv_Queue_List[118] = &Queue_UE_data_recv_118;
    UE_data_recv_Queue_List[119] = &Queue_UE_data_recv_119;
    UE_data_recv_Queue_List[120] = &Queue_UE_data_recv_120;
    UE_data_recv_Queue_List[121] = &Queue_UE_data_recv_121;
    UE_data_recv_Queue_List[122] = &Queue_UE_data_recv_122;
    UE_data_recv_Queue_List[123] = &Queue_UE_data_recv_123;
    UE_data_recv_Queue_List[124] = &Queue_UE_data_recv_124;
    UE_data_recv_Queue_List[125] = &Queue_UE_data_recv_125;
    UE_data_recv_Queue_List[126] = &Queue_UE_data_recv_126;
    UE_data_recv_Queue_List[127] = &Queue_UE_data_recv_127;
    UE_data_recv_Queue_List[128] = &Queue_UE_data_recv_128;
    UE_data_recv_Queue_List[129] = &Queue_UE_data_recv_129;
    UE_data_recv_Queue_List[130] = &Queue_UE_data_recv_130;
    UE_data_recv_Queue_List[131] = &Queue_UE_data_recv_131;
    UE_data_recv_Queue_List[132] = &Queue_UE_data_recv_132;
    UE_data_recv_Queue_List[133] = &Queue_UE_data_recv_133;
    UE_data_recv_Queue_List[134] = &Queue_UE_data_recv_134;
    UE_data_recv_Queue_List[135] = &Queue_UE_data_recv_135;
    UE_data_recv_Queue_List[136] = &Queue_UE_data_recv_136;
    UE_data_recv_Queue_List[137] = &Queue_UE_data_recv_137;
    UE_data_recv_Queue_List[138] = &Queue_UE_data_recv_138;
    UE_data_recv_Queue_List[139] = &Queue_UE_data_recv_139;
    UE_data_recv_Queue_List[140] = &Queue_UE_data_recv_140;
    UE_data_recv_Queue_List[141] = &Queue_UE_data_recv_141;
    UE_data_recv_Queue_List[142] = &Queue_UE_data_recv_142;
    UE_data_recv_Queue_List[143] = &Queue_UE_data_recv_143;
    UE_data_recv_Queue_List[144] = &Queue_UE_data_recv_144;
    UE_data_recv_Queue_List[145] = &Queue_UE_data_recv_145;
    UE_data_recv_Queue_List[146] = &Queue_UE_data_recv_146;
    UE_data_recv_Queue_List[147] = &Queue_UE_data_recv_147;
    UE_data_recv_Queue_List[148] = &Queue_UE_data_recv_148;
    UE_data_recv_Queue_List[149] = &Queue_UE_data_recv_149;
    UE_data_recv_Queue_List[150] = &Queue_UE_data_recv_150;
    UE_data_recv_Queue_List[151] = &Queue_UE_data_recv_151;
    UE_data_recv_Queue_List[152] = &Queue_UE_data_recv_152;
    UE_data_recv_Queue_List[153] = &Queue_UE_data_recv_153;
    UE_data_recv_Queue_List[154] = &Queue_UE_data_recv_154;
    UE_data_recv_Queue_List[155] = &Queue_UE_data_recv_155;
    UE_data_recv_Queue_List[156] = &Queue_UE_data_recv_156;
    UE_data_recv_Queue_List[157] = &Queue_UE_data_recv_157;
    UE_data_recv_Queue_List[158] = &Queue_UE_data_recv_158;
    UE_data_recv_Queue_List[159] = &Queue_UE_data_recv_159;
    UE_data_recv_Queue_List[160] = &Queue_UE_data_recv_160;
    UE_data_recv_Queue_List[161] = &Queue_UE_data_recv_161;
    UE_data_recv_Queue_List[162] = &Queue_UE_data_recv_162;
    UE_data_recv_Queue_List[163] = &Queue_UE_data_recv_163;
    UE_data_recv_Queue_List[164] = &Queue_UE_data_recv_164;
    UE_data_recv_Queue_List[165] = &Queue_UE_data_recv_165;
    UE_data_recv_Queue_List[166] = &Queue_UE_data_recv_166;
    UE_data_recv_Queue_List[167] = &Queue_UE_data_recv_167;
    UE_data_recv_Queue_List[168] = &Queue_UE_data_recv_168;
    UE_data_recv_Queue_List[169] = &Queue_UE_data_recv_169;
    UE_data_recv_Queue_List[170] = &Queue_UE_data_recv_170;
    UE_data_recv_Queue_List[171] = &Queue_UE_data_recv_171;
    UE_data_recv_Queue_List[172] = &Queue_UE_data_recv_172;
    UE_data_recv_Queue_List[173] = &Queue_UE_data_recv_173;
    UE_data_recv_Queue_List[174] = &Queue_UE_data_recv_174;
    UE_data_recv_Queue_List[175] = &Queue_UE_data_recv_175;
    UE_data_recv_Queue_List[176] = &Queue_UE_data_recv_176;
    UE_data_recv_Queue_List[177] = &Queue_UE_data_recv_177;
    UE_data_recv_Queue_List[178] = &Queue_UE_data_recv_178;
    UE_data_recv_Queue_List[179] = &Queue_UE_data_recv_179;
    UE_data_recv_Queue_List[180] = &Queue_UE_data_recv_180;
    UE_data_recv_Queue_List[181] = &Queue_UE_data_recv_181;
    UE_data_recv_Queue_List[182] = &Queue_UE_data_recv_182;
    UE_data_recv_Queue_List[183] = &Queue_UE_data_recv_183;
    UE_data_recv_Queue_List[184] = &Queue_UE_data_recv_184;
    UE_data_recv_Queue_List[185] = &Queue_UE_data_recv_185;
    UE_data_recv_Queue_List[186] = &Queue_UE_data_recv_186;
    UE_data_recv_Queue_List[187] = &Queue_UE_data_recv_187;
    UE_data_recv_Queue_List[188] = &Queue_UE_data_recv_188;
    UE_data_recv_Queue_List[189] = &Queue_UE_data_recv_189;
    UE_data_recv_Queue_List[190] = &Queue_UE_data_recv_190;
    UE_data_recv_Queue_List[191] = &Queue_UE_data_recv_191;
    UE_data_recv_Queue_List[192] = &Queue_UE_data_recv_192;
    UE_data_recv_Queue_List[193] = &Queue_UE_data_recv_193;
    UE_data_recv_Queue_List[194] = &Queue_UE_data_recv_194;
    UE_data_recv_Queue_List[195] = &Queue_UE_data_recv_195;
    UE_data_recv_Queue_List[196] = &Queue_UE_data_recv_196;
    UE_data_recv_Queue_List[197] = &Queue_UE_data_recv_197;
    UE_data_recv_Queue_List[198] = &Queue_UE_data_recv_198;
    UE_data_recv_Queue_List[199] = &Queue_UE_data_recv_199;
    UE_data_recv_Queue_List[200] = &Queue_UE_data_recv_200;
    UE_data_recv_Queue_List[201] = &Queue_UE_data_recv_201;
    UE_data_recv_Queue_List[202] = &Queue_UE_data_recv_202;
    UE_data_recv_Queue_List[203] = &Queue_UE_data_recv_203;
    UE_data_recv_Queue_List[204] = &Queue_UE_data_recv_204;
    UE_data_recv_Queue_List[205] = &Queue_UE_data_recv_205;
    UE_data_recv_Queue_List[206] = &Queue_UE_data_recv_206;
    UE_data_recv_Queue_List[207] = &Queue_UE_data_recv_207;
    UE_data_recv_Queue_List[208] = &Queue_UE_data_recv_208;
    UE_data_recv_Queue_List[209] = &Queue_UE_data_recv_209;
    UE_data_recv_Queue_List[210] = &Queue_UE_data_recv_210;
    UE_data_recv_Queue_List[211] = &Queue_UE_data_recv_211;
    UE_data_recv_Queue_List[212] = &Queue_UE_data_recv_212;
    UE_data_recv_Queue_List[213] = &Queue_UE_data_recv_213;
    UE_data_recv_Queue_List[214] = &Queue_UE_data_recv_214;
    UE_data_recv_Queue_List[215] = &Queue_UE_data_recv_215;
    UE_data_recv_Queue_List[216] = &Queue_UE_data_recv_216;
    UE_data_recv_Queue_List[217] = &Queue_UE_data_recv_217;
    UE_data_recv_Queue_List[218] = &Queue_UE_data_recv_218;
    UE_data_recv_Queue_List[219] = &Queue_UE_data_recv_219;
    UE_data_recv_Queue_List[220] = &Queue_UE_data_recv_220;
    UE_data_recv_Queue_List[221] = &Queue_UE_data_recv_221;
    UE_data_recv_Queue_List[222] = &Queue_UE_data_recv_222;
    UE_data_recv_Queue_List[223] = &Queue_UE_data_recv_223;
    UE_data_recv_Queue_List[224] = &Queue_UE_data_recv_224;
    UE_data_recv_Queue_List[225] = &Queue_UE_data_recv_225;
    UE_data_recv_Queue_List[226] = &Queue_UE_data_recv_226;
    UE_data_recv_Queue_List[227] = &Queue_UE_data_recv_227;
    UE_data_recv_Queue_List[228] = &Queue_UE_data_recv_228;
    UE_data_recv_Queue_List[229] = &Queue_UE_data_recv_229;
    UE_data_recv_Queue_List[230] = &Queue_UE_data_recv_230;
    UE_data_recv_Queue_List[231] = &Queue_UE_data_recv_231;
    UE_data_recv_Queue_List[232] = &Queue_UE_data_recv_232;
    UE_data_recv_Queue_List[233] = &Queue_UE_data_recv_233;
    UE_data_recv_Queue_List[234] = &Queue_UE_data_recv_234;
    UE_data_recv_Queue_List[235] = &Queue_UE_data_recv_235;
    UE_data_recv_Queue_List[236] = &Queue_UE_data_recv_236;
    UE_data_recv_Queue_List[237] = &Queue_UE_data_recv_237;
    UE_data_recv_Queue_List[238] = &Queue_UE_data_recv_238;
    UE_data_recv_Queue_List[239] = &Queue_UE_data_recv_239;
    UE_data_recv_Queue_List[240] = &Queue_UE_data_recv_240;
    UE_data_recv_Queue_List[241] = &Queue_UE_data_recv_241;
    UE_data_recv_Queue_List[242] = &Queue_UE_data_recv_242;
    UE_data_recv_Queue_List[243] = &Queue_UE_data_recv_243;
    UE_data_recv_Queue_List[244] = &Queue_UE_data_recv_244;
    UE_data_recv_Queue_List[245] = &Queue_UE_data_recv_245;
    UE_data_recv_Queue_List[246] = &Queue_UE_data_recv_246;
    UE_data_recv_Queue_List[247] = &Queue_UE_data_recv_247;
    UE_data_recv_Queue_List[248] = &Queue_UE_data_recv_248;
    UE_data_recv_Queue_List[249] = &Queue_UE_data_recv_249;

    u8 queue_index = 0;
    for (queue_index; queue_index < BBU_UE_MAXNUM; queue_index++)
    {
        App_queue_init(UE_data_recv_Queue_List[queue_index]);
    }
}

// 记录构成发往物理层帧的中间过程结构体数组的初始化
void init_Mac_to_PHY_data()
{
    int index;
    index = 0;
    for (index; index < PER_WAVE_MAX_NUM; index++)
    {
        Mac_to_PHY_data[index].CCH_RB_begin = 0xFF;
        Mac_to_PHY_data[index].CCH_frame_head = 0xFFFF;
        memset(Mac_to_PHY_data[index].CCH_DEST_MAC.value, 0xFF, MY_MAC_ADDR_BYTE_NUM);
        memset(Mac_to_PHY_data[index].CCH_SRC_MAC.value, 0xFF, MY_MAC_ADDR_BYTE_NUM);

        memset(Mac_to_PHY_data[index].CCH_DATA, 0, CCH_DATA_LENGTH);

        Mac_to_PHY_data[index].SCH_MCS = 0xFF;
        Mac_to_PHY_data[index].SCH_TBS = 0xFFFF;
        Mac_to_PHY_data[index].SCH_RB_Begin = 0xFFFF;
        Mac_to_PHY_data[index].SCH_RB_Length = 0xFFFF;
        Mac_to_PHY_data[index].SCH_frame_head = 0xFFFF;
        memset(Mac_to_PHY_data[index].SCH_DEST_MAC.value, 0xFF, MY_MAC_ADDR_BYTE_NUM);
        memset(Mac_to_PHY_data[index].SCH_SRC_MAC.value, 0xFF, MY_MAC_ADDR_BYTE_NUM);
    }
}
void init_RB_Reception_Info()
{
    int index;
    index = 0;
    for (index; index < PER_WAVE_MAX_NUM; index++)
    {
        RB_Reception_Info[index].CCH_RB_begin = 0xFF;

        RB_Reception_Info[index].SCH_MCS = 0xFF;
        RB_Reception_Info[index].SCH_TBS = 0xFFFF;
        RB_Reception_Info[index].SCH_RB_Begin = 0xFFFF;
        RB_Reception_Info[index].SCH_RB_Length = 0xFFFF;
    }
}