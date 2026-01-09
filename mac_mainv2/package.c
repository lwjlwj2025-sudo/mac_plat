#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "package.h"
#include "App_queue.h"
#include "App_udp.h"
#include "globals.h"
#include "CONSTANTS.h"
#include "config.h"
#include "mac_process.h"
#include "info_process.h"
u32 TYPE_RARSP_Handle(msg_all *msg);
u32 TYPE_RAMSG_Handle(msg_all *msg);
u32 TYPE_UE_OnlineReport_Handle(msg_all *msg);
u32 TYPE_UE_DataSum_Handle(msg_all *msg);
u32 TYPE_Sat_DataSum_Handle(msg_all *msg);
u32 TYPE_MSG_division_Handle(msg_all *msg);
u32 TYPE_DIV_NONE_Handle(msg_all *msg);
u32 TYPE_DIV_HEAD_Handle(msg_all *msg);
u32 TYPE_DIV_MID_Handle(msg_all *msg);
u32 TYPE_DIV_END_Handle(msg_all *msg);

// u32 msgHandle(msg_all *msg);

/*
函数将一个unsigned char类型的数组从小端转换为大端,length为数组有效长度
unsigned char input[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
size_t length = sizeof(input) / sizeof(input[0]);
*/

void littleEndianToBigEndian(const unsigned char *input, unsigned char *output, size_t length)
{
    size_t i;
    i = 0;
    for (i; i < length; i++)
    {
        output[length - 1 - i] = input[i];
    }
}

// 16位大小端转换
u16 swap16(u16 value)
{
    return (value >> 8) | (value << 8);
}

// 32位大小端转换
u32 swap32(u32 value)
{
    return ((value >> 24) & 0xFF) | ((value >> 8) & 0xFF00) |
           ((value << 8) & 0xFF0000) | ((value << 24) & 0xFF000000);
}

Msg_Recvhandle_s_type RRC_Recvhandle_Table[] =
    {
        {TYPE_RAMSG, TYPE_RAMSG_Handle},
};

Msg_Recvhandle_s_type RLC_Recvhandle_Table[] =
    {
        /*皇甫阳阳*/
        //{TYPE_SAT_DATA_SUM,TYPE_Sat_DataSum_Handle},
        {TYPE_MSG_DIVISION, TYPE_MSG_division_Handle},
};

Msg_Recvhandle_s_type MAC_U_Recvhandle_Table[] =
    {

        {TYPE_UE_OnlineReport, TYPE_UE_OnlineReport_Handle},
        {TYPE_UE_DATA_SUM, TYPE_UE_DataSum_Handle},
        /*皇甫阳阳*/
        {TYPE_NON_RESIDENT_DATA_SUM, TYPE_UE_NON_RESIDENT_Handle},
        {TYPE_DIV_NONE, TYPE_DIV_NONE_Handle},
        {TYPE_DIV_HEAD, TYPE_DIV_HEAD_Handle},
        {TYPE_DIV_MID, TYPE_DIV_MID_Handle},
        {TYPE_DIV_END, TYPE_DIV_END_Handle},
};

u32 TYPE_RAMSG_Handle(msg_all *msg)
{
    LOG("Sat_mac_u recv RAMSG");
    RAMSG_process(msg);

    // stimer_queue_add_us("mac_gen_phy_to_mac_pk", gen_phy_to_mac_pk, NULL, 0, 2000000);
    return OK;
}

u32 TYPE_RARSP_Handle(msg_all *msg)
{
    return OK;
}

u32 TYPE_UE_OnlineReport_Handle(msg_all *msg)
{
    LOG("mac-u recv OnlineReport");
    UE_OnlineReport_process(msg);
    return OK;
}

u32 TYPE_UE_DataSum_Handle(msg_all *msg)
{
    LOG("mac-u recv Data_sum");
    UE_DataSum_process(msg);
    return OK;
}
/*皇甫阳阳*/
u32 TYPE_UE_NON_RESIDENT_Handle(msg_all *msg)
{
    LOG("mac-u recv non_resident_data_sum");
    // non_resident_resource_apply(msg);
    return OK;
}

u32 TYPE_Sat_DataSum_Handle(msg_all *msg)
{
    LOG("mac-u recv Data_sum");
    Sat_DataSum_process(msg);
    return OK;
}

u32 TYPE_DIV_NONE_Handle(msg_all *msg)
{
    type_div_none_s *div_none = (type_div_none_s *)msg->HEAD->DATA;

    u8 data[128] = {0};
    u16 data_len = div_none->Res_len;
    memcpy(data, div_none->Data, data_len);

    u8 package[256] = {0};
    u16 package_len = Package_Frame(TYPE_MSG_DIVISION, data, data_len, package, sizeof(package));

    UDP_write_rlc(package, package_len);
    return OK;
}

u32 TYPE_DIV_HEAD_Handle(msg_all *msg)
{
    type_div_head_s *div_head = (type_div_head_s *)msg->HEAD->DATA;
    u32 get_ue_rnti = div_head->URNTI;
    u16 ue_local_index = RNTI_to_LocalIndex_map(get_ue_rnti, BBU_index);
    App_queue_push(UE_data_recv_Queue_List[ue_local_index], (char *)div_head, DIV_DATA_LEN);
    return OK;
}

u32 TYPE_DIV_MID_Handle(msg_all *msg)
{
    type_div_mid_s *div_mid = (type_div_mid_s *)msg->HEAD->DATA;
    DIV_MID_Handle(div_mid);
    return OK;
}

u32 TYPE_DIV_END_Handle(msg_all *msg)
{
    type_div_end_s *div_end = (type_div_end_s *)msg->HEAD->DATA;
    DIV_END_Handle(div_end);
    return OK;
}

// 发送组包
u32 Package_Frame(u8 type, u8 *data, u16 data_len, u8 *buff, u16 buffLen)
{
    msg_all package_frame;
    u32 frameLen = sizeof(msg_head) + sizeof(msg_tail) + data_len;

    if (buffLen < frameLen)
    {
        LOG("buffLen is not enough.\r\n");
        return -1;
    }

    package_frame.HEAD = (msg_head *)buff;

    package_frame.HEAD->HEAD_ID = FRAME_HEAD;
    package_frame.HEAD->TYPE = type;
    package_frame.HEAD->LEN = data_len;
    memcpy(package_frame.HEAD->DATA, data, data_len);

    package_frame.TAIL = (msg_tail *)&(package_frame.HEAD->DATA[data_len]);
    package_frame.TAIL->TAIL_ID = FRAME_TAIL;

    //    LOG("package mac frame done.");
    return frameLen;
}

// 获取整包长度
u32 getMacFrameLen(msg_all *frame)
{
    return sizeof(msg_head) + sizeof(msg_tail) + frame->HEAD->LEN;
}

// 流程：入队、拆包、校验、处理
u16 recvPackageHandle(u8 *recv_buff, u16 recv_buff_len, App_queue *Client_Queue, u32 (*RecvHandle)(msg_all *))
{
    if (QUEUE_OK != App_queue_push(Client_Queue, recv_buff, recv_buff_len)) // 如果入队失败，再入队，确保进入
    {
        // LOG("queue push failed, pop %d bytes.", recv_buff_len);
        App_queue_init(Client_Queue);
        App_queue_push(Client_Queue, recv_buff, recv_buff_len);
    }

    u8 buffer[QUEMAXSIZE] = {0};
    // i：包头索引 j：包尾索引 k：拆出的包数量
    u32 i, j = 0, k = 0;
    u32 queue_size = App_queue_size(Client_Queue);
    u32 remainQueueSize = queue_size;
    App_queue_pop(Client_Queue, buffer, queue_size, 0x01);
    msg_all frame = {0};

    // u32 offset = 0;
    for (i = 0; i < queue_size; i++)
    {
        if (sizeof(msg_head) + sizeof(msg_tail) >= remainQueueSize)
        {
            break;
        }

        frame.HEAD = (msg_head *)&buffer[i];
        if (frame.HEAD->HEAD_ID == FRAME_HEAD)
        {

            u32 MacFrameLen = getMacFrameLen(&frame);
            if (MacFrameLen > remainQueueSize)
            {
                continue;
            }
            frame.HEAD->LEN = frame.HEAD->LEN;
            frame.TAIL = (msg_tail *)&frame.HEAD->DATA[frame.HEAD->LEN];
            if (frame.TAIL->TAIL_ID == FRAME_TAIL)
            {
                i += MacFrameLen - 1;
                j = i + 1;
                k++;
                remainQueueSize -= MacFrameLen;
                RecvHandle(&frame); // 处理数据
                // if (frame.HEAD->LEN < 500)
                // {
                //     memcpy(recv_package + offset, frame.HEAD->DATA, frame.HEAD->LEN);
                //     offset += frame.HEAD->LEN;
                // }

                printf("data recv success \n");
                // RecvHandle(&frame);
            }
        }
    }

    // recv_package_len = offset;
    //  printf("%d %d\n",offset,recv_package_len);
    if (j)
    {
        App_queue_pop(Client_Queue, NULL, j, 0x02);
    }
    return 0;
}

/*皇甫阳阳*/
// 加入包头识别码 0101 0101 0101 0101
u16 SCH_DATA_Packaging(u8 *data, u16 data_len, u8 *buffer)
{
    mac_msg_type *mac_frame;
    mac_frame = (mac_msg_type *)buffer;
    mac_frame->FramHead = PK_FRAMEHEAD;
    mac_frame->DataLen = data_len; // 包长
    memcpy(mac_frame->DataField, data, data_len);

    return (data_len + sizeof(mac_msg_type));
}
/*****************/

u32 from_sat_rrc_msgHandle(msg_all *msg)
{
    LOG("SAT MACU 从RRC收到数据\n");
    int i;
    i = 0;
    for (i; i < TABLE_ARRSIZE(RRC_Recvhandle_Table); i++)
    {
        if (msg->HEAD->TYPE == RRC_Recvhandle_Table[i].msgType)
        {
            LOG("Find the msgType: 0x%d.", msg->HEAD->TYPE);
            if (OK != RRC_Recvhandle_Table[i].Handler(msg))
            {
                LOG("Handle msg failed.");
            }
            else
            {
                LOG("Handle msg success.");
            }
            return OK;
        }
    }
}

u32 from_sat_rlc_msgHandle(msg_all *msg)
{
    LOG("MACU REC DATA FROM RLC\n");
    int i;
    i = 0;
    for (i; i < TABLE_ARRSIZE(RLC_Recvhandle_Table); i++)
    {
        if (msg->HEAD->TYPE == RLC_Recvhandle_Table[i].msgType)
        {
            // LOG("Find the msgType: 0x%d.", msg->HEAD->TYPE);
            if (OK != RLC_Recvhandle_Table[i].Handler(msg))
            {
                LOG("Handle msg failed.");
            }
            else
            {
                LOG("Handle msg success.");
            }
            return OK;
        }
    }
}

u32 from_ue_mac_u_msgHandle(msg_all *msg)
{
    int i;
    i = 0;
    for (i; i < TABLE_ARRSIZE(MAC_U_Recvhandle_Table); i++)
    {
        if (msg->HEAD->TYPE == MAC_U_Recvhandle_Table[i].msgType)
        {
            // LOG("Find the msgType: 0x%d.", msg->HEAD->TYPE);
            if (OK != MAC_U_Recvhandle_Table[i].Handler(msg))
            {
                // LOG("Handle msg failed.");
            }
            else
            {
                // LOG("Handle msg success.");
            }
            return OK;
        }
    }
}
// Package_MIB1组包
u32 Package_MIB1(u8 *data, u16 data_len, u8 *buff, u16 buffLen)
{
    type_MIB1_s *MIB1_frame = (type_MIB1_s *)buff;

    MIB1_frame->CID = MIB1_CID;
    memcpy(MIB1_frame->Sat_MAC.value, own_address.value, MY_MAC_ADDR_BYTE_NUM);
    memcpy(MIB1_frame->Data, data, data_len);
    MIB1_frame->Checknum = 0;

    return sizeof(type_MIB1_s);
}

// Package_MIB2组包
u32 Package_MIB2(u8 *buff, u16 buffLen)
{
    type_MIB2_s *MIB2_frame = (type_MIB2_s *)buff;

    MIB2_frame->CID = MIB2_CID;
    memcpy(MIB2_frame->Sat_MAC.value, own_address.value, MY_MAC_ADDR_BYTE_NUM);
    MIB2_frame->Downlink_mask = 1;
    MIB2_frame->Frame_num = 1;
    MIB2_frame->Timeslot_num = 1;
    MIB2_frame->Uplink_mask = 1;
    MIB2_frame->Beam_num = 1;
    MIB2_frame->Access_inipara = 1;
    MIB2_frame->Access_timer = 1;
    MIB2_frame->Downlink_info = 1;
    MIB2_frame->Uplink_info = 1;
    MIB2_frame->RES = 1;
    MIB2_frame->Checknum = 1;

    return sizeof(type_MIB2_s);
}

// Package_RAREQ组包
u32 Package_RAREQ(u8 *buff, u16 buffLen)
{
    type_RAREQ_s *RAREQ_frame = (type_RAREQ_s *)buff;

    RAREQ_frame->CID = RAREQ_CID;
    memcpy(RAREQ_frame->SatMAC.value, own_address.value, MY_MAC_ADDR_BYTE_NUM);
    memcpy(RAREQ_frame->UEMAC.value, own_address.value, MY_MAC_ADDR_BYTE_NUM);
    RAREQ_frame->Wave_num = 0x01;
    RAREQ_frame->Auth_info = 0x01;
    RAREQ_frame->Checknum = 0x01;

    return sizeof(type_RAREQ_s);
}

// Package_RAMSG组包
u32 Package_RAMSG(u8 *buff, u16 buffLen)
{
    type_RAMSG_s *RAMSG_frame = (type_RAMSG_s *)buff;

    RAMSG_frame->CID = RAMSG_CID;
    memcpy(RAMSG_frame->UEMAC.value, own_address.value, MY_MAC_ADDR_BYTE_NUM);
    RAMSG_frame->Wave_num = 0x01;
    RAMSG_frame->URNTI = 65536;

    return sizeof(type_RAMSG_s);
}

// Package_RARSP组包
u32 Package_RARSP(u8 *buff, u16 buffLen)
{
    type_RARSP_s *RARSP_frame = (type_RARSP_s *)buff;

    RARSP_frame->CID = RARSP_CID;
    memcpy(RARSP_frame->UEMAC.value, own_address.value, MY_MAC_ADDR_BYTE_NUM);
    memcpy(RARSP_frame->UEMAC.value, own_address.value, MY_MAC_ADDR_BYTE_NUM);
    RARSP_frame->URNTI = 0x01;
    RARSP_frame->Uplink_num = 0x01;
    RARSP_frame->Uplink_fre = 0x01;
    RARSP_frame->Uplink_mode = 0x01;
    RARSP_frame->Downlink_num = 0x01;
    RARSP_frame->Downlink_fre = 0x01;
    RARSP_frame->Downlink_mode = 0x01;

    return sizeof(type_RARSP_s);
}

// Package_UE_OnlineReport组包
u32 Package_UE_OnlineReport(u8 *buff, u16 buffLen)
{
    type_UE_OnlineReport_s *UE_OnlineReport_frame = (type_UE_OnlineReport_s *)buff;
    UE_OnlineReport_frame->CID = UE_OnlineReport_CID;
    memcpy(UE_OnlineReport_frame->SatMAC.value, own_address.value, MY_MAC_ADDR_BYTE_NUM);
    memcpy(UE_OnlineReport_frame->UEMAC.value, own_address.value, MY_MAC_ADDR_BYTE_NUM);
    UE_OnlineReport_frame->URNTI = 0x01;
    UE_OnlineReport_frame->RRC_State = 1;
    return sizeof(type_UE_OnlineReport_s);
}

// Package_Sat_OnlineReport组包
u32 Package_Sat_OnlineReport(u8 *buff, u16 buffLen)
{
    type_Sat_OnlineReport_s *Sat_OnlineReport_frame = (type_Sat_OnlineReport_s *)buff;
    Sat_OnlineReport_frame->CID = Sat_OnlineReport_CID;
    memcpy(Sat_OnlineReport_frame->SatMAC.value, own_address.value, MY_MAC_ADDR_BYTE_NUM);
    Sat_OnlineReport_frame->beam_num = 0x01;
    Sat_OnlineReport_frame->wave_num = 0x01;
    return sizeof(type_Sat_OnlineReport_s);
}

// 暂定先不拆包
u32 TYPE_MSG_division_Handle(msg_all *msg)
{
    LOG("SAT MACU rec Data!\n");

    type_msg_division_s *msg_division = (type_msg_division_s *)msg->HEAD->DATA;

    char buffer[1024] = {0};
    u16 DataLen_Record = SCH_DATA_Packaging((u8 *)msg_division, (sizeof(type_msg_division_s) + msg_division->Data_len), buffer);
    /*皇甫阳阳*/
    // MSG_division_Handle(msg_division);
    // 解析数据包，获取数据包对应的目标用户的IP,用户计算目标用户的基带板序号
    u32 tmp_ue_ip;
    u8 bbu_ue_index;

    tmp_ue_ip = msg_division->Dest_IP;
    bbu_ue_index = (tmp_ue_ip & (0x000000FF));
    LOG("bbu_ue_index = %d", bbu_ue_index);

    // 插队列
    cvector_pushback(DL_data_buffer_Cvector_List[bbu_ue_index], buffer, DataLen_Record);
    // App_queue_push(DL_data_buffer_Queue_List[bbu_ue_index], msg_division, (sizeof(type_msg_division_s) + msg_division->Data_len));
    /********************/
    return OK;
}

void CCH_unpacking(u8 *data_buffer, u16 size) // 检查cch的rrc是否连接正常
{
    u16 i = 0;
    u8 tmp_cid;

    while (i < size)
    {
        mac_msg_type *cch_data = (mac_msg_type *)(&data_buffer[i]);
        if (cch_data->FramHead == PK_FRAMEHEAD)
        {
            i += sizeof(mac_msg_type);
            tmp_cid = data_buffer[i];
            LOG("CCH_cid = %x\n", tmp_cid);
            if (tmp_cid == TYPE_UE_OnlineReport)
            {
                type_UE_OnlineReport_s *UE_OnlineReport_frame = (type_UE_OnlineReport_s *)(&data_buffer[i]);

                LOG("UE MACU：解析用户在线状态报告\n");
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
                i += sizeof(type_UE_OnlineReport_s);
            }
            else if (tmp_cid == TYPE_NON_RESIDENT_DATA_SUM)
            {
                type_non_resident_data_sum_s *UE_non_resident_DataSum_frame = (type_non_resident_data_sum_s *)(&data_buffer[i]);
                non_resident_resource_apply(UE_non_resident_DataSum_frame);
                i += sizeof(type_non_resident_data_sum_s);
            }

            else
            {
                LOG("不正确的控制包!!\n");
            }
        }
        else
        {
            i++;
        }
    }
}

void SCH_unpacking(u8 *data_buffer, u16 size)
{
    u16 i = 0;
    u8 tmp_cid;

    while (i < size)
    {
        mac_msg_type *sch_data = (mac_msg_type *)(&data_buffer[i]);
        // LOG("sch_data->FramHead = %04x", sch_data->FramHead);
        if (sch_data->FramHead == PK_FRAMEHEAD) // 如果数据包的framehead不匹配
        {
            i += sizeof(mac_msg_type);
            type_msg_division_s *data = (type_msg_division_s *)(&data_buffer[i]);
            u16 sch_udp_length = 0;
            u8 sch_udp_buffer[512];
            sch_udp_length = Package_Frame(TYPE_MSG_DIVISION, (u8 *)data, data->Data_len, sch_udp_buffer, 512);
            UDP_write_rlc(sch_udp_buffer, sch_udp_length);
            LOG("往上层传业务包\n");
            i += data->Data_len;
        }
        else
        {
            i++;
        }
    }
}

// PHY package handle

u32 recvphyPackageHandle(u8 *buffer, u16 recv_buff_len, App_queue *Client_Queue)
{
    u32 index;
    index = 0;
    for (index; index < recv_buff_len; index++)
    {
        printf("%02x ", buffer[index]);
    }
    printf("\n");
    char *recv_buff = (char *)buffer;
    LOG("recv_buff_len = %d", recv_buff_len);
    if (QUEUE_OK != App_queue_push(Client_Queue, recv_buff, recv_buff_len))
    {
        // LOG("queue push failed, pop %d bytes.", recv_buff_len);
        App_queue_init(Client_Queue);
        App_queue_push(Client_Queue, recv_buff, recv_buff_len);
    }
    // u8 buffer[QUEMAXSIZE] = {0};
    // i：包头索引 j：包尾索引 k：拆出的包数量
    u32 i = 0;
    // u32 queue_size = App_queue_size(Client_Queue);
    // App_queue_pop(Client_Queue, buffer, queue_size, 0x03);
    mac_head_type *tmp_frameHead = {0};

    while (i < recv_buff_len)
    {
        LOG("i = %d", i);
        tmp_frameHead = (mac_head_type *)&(buffer[i]);
        if (strcmp(tmp_frameHead->Dest_MAC, own_address.value) == 0) // 检查数据包的mac地址和自己的ac地址是否一致，调用相应解包函数,就是从用户端发送来的数据
        {
            if (tmp_frameHead->FramHead == CCH_FRAMEHEAD)
            {
                CCH_unpacking(&buffer[i + sizeof(mac_head_type)], tmp_frameHead->Data_Length);
                i += CCH_DATA_LENGTH; // 跳过处理的数据长度
            }
            else if (tmp_frameHead->FramHead == SCH_FRAMEHEAD)
            {
                SCH_unpacking(&buffer[i + sizeof(mac_head_type)], tmp_frameHead->Data_Length); // 有一步是往上层传业务包
                i += (tmp_frameHead->Data_Length + sizeof(mac_head_type));
                LOG("(tmp_frameHead->Data_Length + sizeof(mac_head_type)) = %lu\n", (tmp_frameHead->Data_Length + sizeof(mac_head_type)));
            }
        }
        else
        {
            i++;
        }
    }
}

// phy->mac
void gen_phy_to_mac_pk() // 生成从物理层到mac层的数据包
{
    u8 UserNum;
    UserNum = 2;
    u16 x_index, y_index;
    x_index = 0;

    u8 pk_buffer[256] = {0};
    u8 big_frame_buff[2000] = {0};
    u16 FrameLength;
    u16 cch_offset;
    u16 sch_offset;
    cch_offset = 0;
    sch_offset = CCH_DATA_LENGTH * UserNum;
    for (x_index; x_index < UserNum; x_index++)
    {
        // 加MAC帧头
        u16 FrameHead = CCH_FRAMEHEAD;
        memcpy(&big_frame_buff[cch_offset], &FrameHead, sizeof(u16));
        cch_offset += sizeof(u16);

        memcpy(&big_frame_buff[cch_offset], own_address.value, sizeof(u48));
        cch_offset += (sizeof(u48) * 2); // 源和目的

        u16 CCH_Data_Length;
        CCH_Data_Length = sizeof(type_UE_OnlineReport_s) + sizeof(type_non_resident_data_sum_s);
        memcpy(&big_frame_buff[cch_offset], &CCH_Data_Length, sizeof(u16));
        cch_offset += sizeof(u16);

        // 生成控制信令
        // 生成用户在线状态报告
        type_UE_OnlineReport_s *ue_online_report = (type_UE_OnlineReport_s *)pk_buffer;
        ue_online_report->CID = TYPE_UE_OnlineReport;
        ue_online_report->RRC_State = 1;
        // 加解大包包用的帧头
        FrameLength = SCH_DATA_Packaging(pk_buffer, sizeof(type_UE_OnlineReport_s), &big_frame_buff[cch_offset]);
        cch_offset += FrameLength;

        // 生成非驻留业务资源申请
        type_non_resident_data_sum_s *non_resident_data_sum = (type_non_resident_data_sum_s *)pk_buffer;
        non_resident_data_sum->CID = TYPE_NON_RESIDENT_DATA_SUM;
        non_resident_data_sum->URNTI = 0;
        non_resident_data_sum->data_length = 2;
        // 加解大包包用的帧头
        FrameLength = SCH_DATA_Packaging(pk_buffer, sizeof(type_non_resident_data_sum_s), &big_frame_buff[cch_offset]);
        cch_offset += FrameLength;
        if (cch_offset % CCH_DATA_LENGTH != 0)
        {
            cch_offset += (CCH_DATA_LENGTH - cch_offset % CCH_DATA_LENGTH);
        }

        // 生成业务
        // 加MAC帧头
        FrameHead = SCH_FRAMEHEAD;
        memcpy(&big_frame_buff[sch_offset], &FrameHead, sizeof(u16));
        sch_offset += sizeof(u16);

        memcpy(&big_frame_buff[sch_offset], own_address.value, sizeof(u48));
        sch_offset += (sizeof(u48) * 2); // 源和目的

        u16 SCH_Data_Length;
        SCH_Data_Length = (sizeof(type_msg_division_s) + 4) * 3;
        memcpy(&big_frame_buff[sch_offset], &SCH_Data_Length, sizeof(u16));
        sch_offset += sizeof(u16);

        y_index = 0;
        for (y_index; y_index < 3; y_index++)
        {
            type_msg_division_s *msg = (type_msg_division_s *)pk_buffer;
            msg->CID = TYPE_MSG_DIVISION;

            FrameLength = SCH_DATA_Packaging(pk_buffer, sizeof(type_msg_division_s), &big_frame_buff[sch_offset]);
            LOG("FrameLength = %d\n", FrameLength);
            sch_offset += FrameLength;
        }
    }
    LOG("收到物理层传来的数据\n");
    recvphyPackageHandle(big_frame_buff, sch_offset, &Queue_UDP_recv_ue_mac_u);
}

/*yinjian*/
// 测试代码
void testSatFramePrintf(u8 *buff)
{
    // userNum_Print_Type* userNum = {0};
    CCH_Print_Type *CCH_Field = {0};
    SCH_Print_Type *SCH_Field = {0};
    // userNum = (userNum_Print_Type*)buff;
    FILE *fp;

    // fp = fopen("/home/snnu_3508/hfyy/Sat/Sat_mac_u/testFrame.txt","w");
    // fprintf(fp,"UserNum = %02x\n", buff[0]);
    LOG("UserNum = %02x\n", buff[0]);
    u8 Num = buff[0]; // 用户数量
    int i;
    int j;
    for (i = 0; i < Num; i++)
    {
        CCH_Field = (CCH_Print_Type *)&(buff[(88 * i + 1)]);
        // fprintf(fp,"CCH_TF_Begin = %02x\n",CCH_Field->CCH_TF_Begin);
        // fprintf(fp,"CCH_Data = \n");
        LOG("CCH_TF_Begin = %d\n", CCH_Field->CCH_TF_Begin);
        LOG("CCH_Data = \n");
        u8 data_cch[1024];
        memcpy(data_cch, CCH_Field->DATA, 87);
        for (j = 0; j < 87; j++)
        {
            // fprintf(fp," %02x ",data_cch[j]);
            printf("%02x ", data_cch[j]);
            // if(j == 86)
            // {
            // 	//fprintf(fp," \n ");
            //     LOG("\n");
            // }
        }
    }
    // fprintf(fp," \n");
    LOG("\n");
    int k;
    int m = 0;
    for (k = 0; k < Num; k++)
    {
        u8 data_sch[2000];

        SCH_Field = (SCH_Print_Type *)&buff[(Num * 88 + 1) + m];
        // fprintf(fp,"SCH_MCS = %02x\n",SCH_Field->SCH_MCS);
        // fprintf(fp,"SCH_TBS = %04x\n",SCH_Field->SCH_TBS);
        // fprintf(fp,"SCH_TF_BEGIN = %04x\n",SCH_Field->SCH_TF_BEGIN);
        // fprintf(fp,"SCH_TF_LENGTH = %04x\n",SCH_Field->SCH_TF_LENGTH);
        // fprintf(fp,"SCH_Data = \n");
        LOG("SCH_MCS = %02x\n", SCH_Field->SCH_MCS);
        LOG("SCH_TBS = %04x\n", SCH_Field->SCH_TBS);
        LOG("SCH_TF_BEGIN = %04x\n", SCH_Field->SCH_TF_BEGIN);
        LOG("SCH_TF_LENGTH = %04x\n", SCH_Field->SCH_TF_LENGTH);
        LOG("SCH_Data = \n");
        memcpy(data_sch, SCH_Field->DATA, SCH_Field->SCH_TF_LENGTH);
        for (j = 0; j < SCH_Field->SCH_TBS; j++)
        {
            // fprintf(fp," %02x ",data_sch[j]);
            printf("%02x ", data_sch[j]);
            // if(j == SCH_Field->SCH_TF_LENGTH - 1)
            // {
            // 	//fprintf(fp, " \n ");
            //     LOG("\n");
            // }
        }
        // fprintf(fp," \n ");
        LOG(" \n");
        m += SCH_Field->SCH_TBS + 7;
    }
    // fclose(fp);
    LOG("测试组帧函数结果完成\n");
}
