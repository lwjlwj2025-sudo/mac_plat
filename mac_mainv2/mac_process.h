#ifndef MAC_PROCESS_H
#define MAC_PROCESS_H
#include "ra_procedure.h"
#include "App_queue.h"
#include "globals.h"
#define MAC_SDU_NUM 10
#define MAX_Logical_num 8
#define MAX_SDUS_PER_CHANNEL 3
#define MAC_PDU_SIZE 100
// #include "LEO_ra_config.h"

extern int online_detect_start_en;
// 在线时间登记表
extern int Online_Time_Table[BBU_UE_MAXNUM];

// 记录各个虚波位的CCH_RB分出去的数量
extern u16 occupied_dlcch_rbnum[WAVE_NUM];

extern u16 cch_rb_id[RB_TOTALNUM];
extern u16 sch_rb_id[SSLOT_TOTALNUM][RB_TOTALNUM];
extern u16 package_totalNum_Array[BBU_UE_MAXNUM];
extern u16 pckage_recvNum_Array[BBU_UE_MAXNUM];
extern u16 pckage_datalen_Array[BBU_UE_MAXNUM];
extern int access_ue_count;
extern int last_detect_time; // 上一次用户在线状态检测的时间，初始化为0
/*皇甫阳阳*/
// 记录还没有分出去的业务RB的最小值
extern u16 DL_min_idleRB;
// 非驻留业务信道资源可用的业务RB的最小值
extern u16 DL_min_flash_available_idleRB;
// 记录还没有分出去的业务RB的最小值
extern u16 UL_min_idleRB;
// 非驻留业务信道资源可用的业务RB的最小值
extern u16 UL_min_flash_available_idleRB;
// 虚波位轮询
// extern u16 poll_wave_index;

// 空闲RB队列管理列表，储存150个队列（queue）的指针，对应150个波位的空闲RB队列，每个队列上限容纳250个RB
extern App_queue *UL_Idle_CCH_RB_Queue_List[WAVE_NUM];
extern App_queue *DL_Idle_CCH_RB_Queue_List[WAVE_NUM];
extern App_queue *UL_Idle_SCH_RB_Queue_List[WAVE_NUM];
extern App_queue *DL_Idle_SCH_RB_Queue_List[WAVE_NUM];

extern App_queue *DL_BusinessPackage_Queue_List[WAVE_NUM];
extern App_queue *DL_ControlPackage_Queue_List[WAVE_NUM];
extern App_queue *UE_data_recv_Queue_List[BBU_UE_MAXNUM];
/*皇甫阳阳*/
extern App_queue *DL_data_buffer_Queue_List[BBU_UE_MAXNUM];
extern cvector *DL_data_buffer_Cvector_List[BBU_UE_MAXNUM];
/****************/
extern UserInfo *UserInfo_PtrArray[BBU_UE_MAXNUM];
extern UserInfo_Block wave_ueser_mapPtrArray[WAVE_NUM][PER_WAVE_MAX_NUM];
/*这俩没有初始化*/
extern SCH_RB_occupiednum *occupiednum_dlsch_rbnum[WAVE_NUM];
extern SCH_RB_idlenum *SCH_RB_idlenum_PtrArray[WAVE_NUM];
/*皇甫阳阳*/
extern u8 per_wave_usernum[WAVE_NUM];
// 记录需要传给物理层的数据
extern Mac_to_PHY Mac_to_PHY_data[PER_WAVE_MAX_NUM];
extern Mac_to_PHY_rec RB_Reception_Info[PER_WAVE_MAX_NUM];

// 开的char类型数组的大小为虚波位用户接满，且业务信息全为最大的情况
extern u8 Mac_to_PHY_send_frame_buffer[PER_WAVE_MAX_NUM * (1031 + 1 + CCH_DATA_LENGTH) + 1];
extern u8 Mac_to_PHY_rec_frame_buffer[PER_WAVE_MAX_NUM * 8 + 1];
/**************/

typedef struct
{
    u8 LCID : 6;
    u8 F : 1;
    u8 R : 1;
    u8 L : 8;
    // u8 DATA[0];
} mac_subheader;
typedef struct
{
    u8 sdusize;
    u8 DATA[0];
} mac_SDU;
typedef struct
{
    u8 LCID : 6;
    u8 R : 2;
    u8 Buffer_Size : 5;
    u8 LCG_ID : 3;
} mac_ce_BSR; // short类型

typedef struct
{
    u8 ccch_length;
    u8 DATA[0];
} mac_ccch_SDU;
typedef struct
{
    u16 datalength;
    u8 LCID;
    u8 DATA[0];
} p_sdu_resource;

// 定义SDU结构体
typedef struct
{
    u8 size; // SDU的大小
    u8 *data;
} RLC_SDU;
typedef struct
{
    u8 frame_num;
    u8 sub_frame_num;
    u8 slot_num;
} frame_info;
extern frame_info *frame_t;
void slot_division();
void update_tokens(Logicalchannel *channel, u32 T);
void sortLevel_1(Logicalchannel channels[], int num_channels);
u16 allocateResources(Logicalchannel channels[], int num_channels, RLC_SDU sdus[][MAX_SDUS_PER_CHANNEL], int num_sdus_per_channel[], p_sdu_resource p_sdu[MAC_SDU_NUM], App_queue *pack_queue);
u16 demultiplex_data(u8 *bitStream);
u16 multiplex_data(App_queue *pack_queue, p_sdu_resource p_sdu[MAC_SDU_NUM]);
void fill_remaining_space(App_queue *packpdu_queue, u8 PDU_SIZE);
void init_Logical(Logicalchannel *channel);
u16 getParamFromBitstream(u8 *bitStream, u16 bitPos, u8 numBits);
u16 cal_RB_carry_capacity(u16 MCS);
void MAC_to_PHY_test();
#endif