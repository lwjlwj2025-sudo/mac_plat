#ifndef PACK_H
#define PACK_H
#include <stdio.h>
#include <string.h>

#include "App_queue.h"
#include "globals.h"

#pragma pack(1) /* 按字节对齐 */

#define TABLE_ARRSIZE(a) (sizeof((a)) / sizeof((a[0])))

/*******************公用段****************************/
// 暂定使用0x0- ~ 0x9F

#define FRAME_HEAD 0x00
#define FRAME_TAIL 0x01
#define TYPE_MIB1 0x02
#define TYPE_MIB2 0x03
#define TYPE_RAREQ 0x04
#define TYPE_RARSP 0x05
#define TYPE_SCH_FILE 0x06
#define TYPE_UE_DATA_SUM 0x07

// 未拆分的业务包相关的
#define TYPE_MSG_DIVISION 0x08
#define TYPE_SRP_MSG 0x09
#define TYPE_IP_MSG 0x0A
#define TYPE_SHT_MSG 0x0B
#define TYPE_SMS_MSG 0x0C
#define TYPE_VADIO_MSG 0x0D
#define TYPE_TONE_MSG 0x0E
#define TYPE_RADIO_MSG 0x0F
#define TYPE_QOS_MSG 0x10

// 拆分后的业务包相关的
#define TYPE_DIV_NONE 0x11
#define TYPE_DIV_HEAD 0x12
#define TYPE_DIV_MID 0x13
#define TYPE_DIV_END 0x14

#define TYPE_SAT_DATA_SUM 0x15

#define TYPE_NON_RESIDENT_DATA_SUM 0x16
#define TYPE_NON_RESIDENT_SCH_FILE 0x17

/******************************UE段***********************************/
// 暂定使用0xA- ~ 0xCF
#define TYPE_DISCOUNT 0xA0
#define TYPE_UE_OnlineReport 0xA2 // 就是SAT侧的TYPE_UE_ONLINE_RE

/******************************SAT段**********************************/
// 暂定使用0xD- ~ 0xFF
// 卫星给波位用户的心跳
#define TYPE_Sat_OnlineReport 0xD0
#define TYPE_RAMSG 0xD1
#define TYPE_Disconnect_MSG 0xD2

#define DATA_MAX1024 1024

#define OK 0
#define ERROR (-1)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef long unsigned int u64;

typedef struct
{
    u8 HEAD_ID;
    u8 TYPE;
    u16 LEN;
    u8 DATA[0];
} msg_head;

typedef struct
{
    u8 TAIL_ID;
} msg_tail;

typedef struct
{
    msg_head *HEAD;
    msg_tail *TAIL;
} msg_all;

typedef struct
{
    u8 CID;
    u48 Sat_MAC;
    u8 Data[44];
    u8 Checknum;
} type_MIB1_s;

typedef struct
{
    u8 CID;
    u48 Sat_MAC;
    u8 Downlink_mask;
    u8 Frame_num;
    u8 Uplink_mask;
    u8 Timeslot_num;
    u8 Beam_num;
    u8 Access_inipara;
    u8 Access_timer;
    u8 Downlink_info;
    u8 Uplink_info;
    u16 RES;
    u8 Checknum;
} type_MIB2_s;

typedef struct
{
    u8 CID;
    u48 SatMAC;
    u48 UEMAC;
    u16 Wave_num;
    u32 Auth_info;
    u8 Checknum;
} type_RAREQ_s;

typedef struct
{
    u8 CID;
    u48 UEMAC;
    u16 Wave_num;
    u32 URNTI;
    u32 IP;
    u8 Service_Type; // 业务类型: 决定 UE_Level
    u8 Access_Type;  // 接入类型: 初始接入 or 调度请求
} type_RAMSG_s;

typedef struct
{
    u8 CID;
    u48 SatMAC;
    u48 UEMAC;
    u32 URNTI;
    u8 Uplink_num;
    u8 Uplink_fre; // 记录分到的连续上行CCH_RB的开始
    u8 Uplink_mode;
    u8 Downlink_num;
    u8 Downlink_fre; // 记录分到的连续下行CCH_RB的开始
    u8 Downlink_mode;
    u32 IP;
} type_RARSP_s;

typedef struct
{
    u8 CID;
    u48 UEMAC;
    u32 URNTI;
    u16 Beam_num;
} type_Disconnect_MSG_s;

typedef struct
{
    u8 CID;
    u48 SatMAC;
    u48 UEMAC;
    u32 URNTI;
    u8 RRC_State;
} type_UE_OnlineReport_s;

typedef struct
{
    u48 UE_MAC_Addr;
    u16 Wave_num;
} type_FromRAREQ_s;

typedef struct
{
    u8 CID;
    u48 SatMAC;
    u16 beam_num;
    u16 wave_num;
} type_Sat_OnlineReport_s;

typedef struct
{
    u8 CID;
    u48 UEMAC;
    u48 SatMAC;
    u32 URNTI;
    u16 Level1_len;
    u16 Level2_len;
    u16 Level3_len;
    u16 Level4_len;
    u16 Level5_len;
    u16 Level6_len;
    u16 Level7_len;
    u16 Level8_len;
} type_data_sum_s;
/*皇甫阳阳*/
typedef struct
{
    u8 CID;
    u48 UEMAC;
    u48 SatMAC;
    u32 URNTI;
    u16 data_length;
} type_non_resident_data_sum_s;

typedef struct
{
    u16 CID;
    u48 SatMAC;
    u48 UEMAC;
    u8 Beam_num;
    u32 URNTI;
    u8 coderate;
    u8 U_or_D;
    u16 RB_NUM_1;
    u16 RB_NUM_2;
    u16 RB_NUM_3;
    u16 RB_NUM_4;
    u16 RB_NUM_5;
    u16 RB_NUM_6;
    u16 RB_NUM_7;
    u16 RB_NUM_8;
    u16 RB_NUM_9;
    u16 RB_NUM_10;
    u16 RB_NUM_11;
    u16 RB_NUM_12;
    u16 RB_NUM_13;
    u16 RB_NUM_14;
    u16 RB_NUM_15;
    u16 RB_NUM_16;
    u16 RB_NUM_17;
    u16 RB_NUM_18;
    u16 RB_NUM_19;
    u16 RB_NUM_20;
} type_SCH_FILE_s;

/*皇甫阳阳*/
typedef struct
{
    u16 CID;
    u48 SatMAC;
    u48 UEMAC;
    u8 Beam_num;
    u32 URNTI;
    u8 U_or_D;
    u16 RB_begin;
    u16 RB_TotalNum;
} type_non_resident_SCH_FILE_s;

typedef struct
{
    u8 CID;
    u8 Data_level;
    u8 UE_level;
    u8 SN;
    u32 Source_IP;
    u32 Dest_IP;
    // u32 Source_MAC          ;
    // u32 Dest_MAC            ;
    u16 Data_len;
    u8 Data[0];
} type_msg_division_s; // 上层给的业务包

typedef struct
{
    u8 CID;
    u32 URNTI;
    u8 DIV; // 0不拆包，1拆包
    u8 Res_len;
    u8 Data[0];
} type_div_none_s;

typedef struct
{
    u8 CID;
    u32 URNTI;
    u8 DIV;       // 0不拆包，1拆包
    u8 Total_Num; // 总共拆了几包
    u8 T_Num;     // 当前是第几包
    u8 Data[0];
} type_div_head_s;

typedef struct
{
    u8 CID;
    u32 URNTI;
    u8 T_Num; // 当前是第几包
    u8 Data[0];
} type_div_mid_s;

typedef struct
{
    u8 CID;
    u32 URNTI;
    u8 T_Num; // 当前是第几包
    u8 Res_len;
    u8 Data[0];
} type_div_end_s;

/*皇甫阳阳*/
/*业务包大包中各个小包的包头*/
typedef struct
{
    u16 FramHead;
    u16 DataLen;
    u8 DataField[0];
} mac_msg_type;
/*yinjian*/
typedef struct
{
    u16 FramHead;
    u8 Dest_MAC[6];
    u8 Src_MAC[6];
    u16 Data_Length;
} mac_head_type;

typedef u32 (*Msg_Recv_handler)(msg_all *msg);

typedef struct
{
    u8 msgType;
    Msg_Recv_handler Handler;
} Msg_Recvhandle_s_type;

u32 Package_Frame(u8 type, u8 *data, u16 data_len, u8 *buff, u16 buffLen);
u16 recvPackageHandle(u8 *recv_buff, u16 recv_buff_len, App_queue *Client_Queue, u32 (*RecvHandle)(msg_all *));
u32 Package_MIB1(u8 *data, u16 data_len, u8 *buff, u16 buffLen);
u32 Package_MIB2(u8 *buff, u16 buffLen);
u32 Package_RAREQ(u8 *buff, u16 buffLen);
u32 Package_RAMSG(u8 *buff, u16 buffLen);
u32 Package_RARSP(u8 *buff, u16 buffLen);
u32 Package_UE_OnlineReport(u8 *buff, u16 buffLen);
u32 Package_Sat_OnlineReport(u8 *buff, u16 buffLen);
u32 from_sat_rrc_msgHandle(msg_all *msg);
u32 from_sat_rlc_msgHandle(msg_all *msg);
u32 from_ue_mac_u_msgHandle(msg_all *msg);
u32 getMacFrameLen(msg_all *frame);

/*皇甫阳阳*/
u16 SCH_DATA_Packaging(u8 *data, u16 data_len, u8 *buffer);
u32 TYPE_UE_NON_RESIDENT_Handle(msg_all *msg);
u32 recvphyPackageHandle(u8 *recv_buff, u16 recv_buff_len, App_queue *Client_Queue);
void CCH_unpacking(u8 *data_buffer, u16 size);
void SCH_unpacking(u8 *data_buffer, u16 size);
void gen_phy_to_mac_pk();
/*yinjian*/
void testSatFramePrintf(u8 *buff);
#endif // PACK_H
