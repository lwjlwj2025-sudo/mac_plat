#ifndef RESOURCE_APPLY_H
#define RESOURCE_APPLY_H
#include "config.h"
#define MAX_LCG_NUM 8
#define LCG_NUM 2
#define SR_PERIOD 2
#define SR_OFFSET 0
// typedef struct
// {
//     u8 *data;
// } T_HeaderInfo;
typedef struct
{
    u8 RIV_1 : 7;
    u8 logo : 1; // 0
    u8 RIV_2 : 8;
    u8 MCS_1 : 3;
    u8 fre_poping : 1;
    u8 time_domain : 4;
    u8 HARQ_1 : 3;
    u8 RV : 2;
    u8 NDI : 1;
    u8 MCS_2 : 2;
    u8 padding : 5;
    u8 TPC : 2;
    u8 HARQ_2 : 1;
} DCI_0_0;
typedef enum
{
    schedule_idle = 0,
    sr_snd = 1,
    bsr_snd = 2,
    data_snd
} resource_apply_state;
typedef struct
{
    resource_apply_state ue_state;
    bool has_data;
    bool is_sr_sent;
    bool is_bsr_sent;
    bool is_grant_received;
    bool is_TB_received;
    u8 sr_periodicity;
    u8 sr_offset;
    u8 sr_trans_max;
    u8 bsr_trans_max;
} ue_resource_apply;
/// Enumeration for parameter SR transmission \ref SCHEDULING_REQUEST_CONFIG::dsr_TransMax.
typedef enum
{
    sr_n4 = 0,
    sr_n8 = 1,
    sr_n16 = 2,
    sr_n32 = 3,
    sr_n64 = 4
} DSR_TRANSMAX_t;

// extern App_queue *waittingdata_queue[];
extern ue_schedule *mac_ue_info;

u8 BSR_TABLE(u8 BS_value);
void init_logical(ue_schedule *mac_ue_info);
u8 *GET_PDCCH_DCI_SR();
u8 *GET_PDCCH_DCI_BSR();
void RLC_MAC_data_test();
void MAC_TO_PHY_SEND_BSR();
void resources_apply();
// void waittingdatabuffer_queue(u8 lcid, u8 pdusize);
u16 trigger_SR(u16 slot_per_frame_num, u8 SR_offset, u8 SR_period, u8 SR_send_num);
u8 send_BSR(DCI_TB_info *ul_grant, u8 LCG_data[], u8 *bsr_to_phy);
u8 *trigger_BSR(u8 short_bsrtrigger_flag, u8 LCG_data[]);
DCI_TB_info *handle_DCI(u8 *buffer);
void DCI_get_from_LEO();
u8 mac_status_logicalgroup_data(u8 LCG_data[]);
#endif