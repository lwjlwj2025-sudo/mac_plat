#ifndef RA_CONFIG_H
#define RA_CONFIG_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "config.h"
#define PREAMBLE_LENGTH 139
#define prach_ConfigIndex 96 //TDD
#define total_preamble_num 63
#define ra_responsewindow 2 //2 subframe
#define ra_preambletransMax 3 //前导传输最大次数
#define rar_transmax 5
#define SSB_num 4
#define SSB_per_prach_occasion 0.25
#define MSG1_FrequencyStart 50
#define msg1_fdm 4
#define RSRP_THresholdSSB 5
#define powerRampingSTEP 2


typedef u16 module_id_t;
typedef enum{
    RA_2STEP=0,
    RA_4STEP
}ra_type;
typedef struct {   //define mac structure
    u16 prach_format;
    u8 RA_PREAMBLE_TRANSMISSION_COUNTER;
    u8 RA_PREAMBLE_POWER_RAMPING_COUNTER;
    u16 power_offset_2step_ra;
    u16 ra_PREAMBLE_RECEIVED_TARGET_POWER;
    u8 ra_TDD_map_index;
    u32 RA_PREAMBLE_POWER_RAMPING_STEP; //dB
    u8 RA_PREAMBLE_BACKOFF;
    u8 RA_SCALING_FACTOR_BI;
    ra_type RA_TYPE; // meiju
    u16 RA_PCMAX;
    u8 f_id;
    u8 t_id;
    u8 s_id; //compute ra_rnti
}PRACH_RESOURCES_t;

typedef struct {
    u16 preambleReceivedTargetPower;
    u8 cfra;
    float ssb_per_prach_occasion;
    u8 totalNumberOfRA_Preamble; //63个用于随机接入，64个前导中有一个前导要用于SI
    u8 CB_preambleperSSB;
    u8 msg1_SubcarrierSpacing;
    u8 rsrp_ThresholdSSB;
    //cfra ssb resource list 
}RACH_Config_rrc; //rrc config
typedef enum {
    RA_UE_IDLE=0,
    msg1_trans=1,
    WAIT_RAR=2,
    receive_rar=3,
    RA_SUCCEESED=4,
    RA_FAILED=5
}RA_state_t;
typedef enum{
    NR_SubcarrierSpacing_kHz15,
    NR_SubcarrierSpacing_kHz30,
    NR_SubcarrierSpacing_kHz60,
    NR_SubcarrierSpacing_kHz120,
    NR_SubcarrierSpacing_kHz240,
    NR_SubcarrierSpacing_kHz480,
    NR_SubcarrierSpacing_kHz960
}msg1_SubcarrierSpacing;
typedef struct {
    RACH_Config_rrc rach_configdedicated;
    msg1_SubcarrierSpacing scs;
    RA_state_t ra_state;
    //u8 cfra;
    u8 RA_offset;
    u16 ra_rnti;
    u16 receivedTargerPower;
    u16 t_crnti; //temporary CRNTI
    u8 RA_attempt_number;
    u8 RA_active;
    /// number of attempt for rach
    /// Random-access preamble index
    int ra_PreambleIndex;
    // When multiple SSBs per RO is configured, this indicates which one is selected in this RO -> this is used to properly compute the PRACH preamble
    u8 ssb_nb_in_ro;
    u8 ssb_per_prach_occasion;
    /// Random-access window counter
    u16 RA_window_cnt;
    /// Flag to monitor if matching RAPID was received in RAR
    u8 RA_RAPID_found;
    /// Flag to monitor if BI was received in RAR
    u8 RA_BI_found;
    /// Random-access backoff counter
    int16_t RA_backoff_indicator;
    /// Flag to indicate whether preambles Group A was used
    u8 RA_usedGroupA;
    /// RA backoff counter
    u16 RA_backoff_cnt;
    /// RA max number of preamble transmissions
    int preambleTransMax;
    /// Nb of preambles per SSB
    long cb_preambles_per_ssb;
    int starting_preamble_nb;
    u16 ra_ResponseWindow;
    u8 preamble_index;
    u8 timing_offset;
    u8 Msg3_TPC;
    PRACH_RESOURCES_t prach_resources;
}RA_config_t;
typedef enum {
  UE_NOT_SYNC = 0,
  UE_SYNC,
  UE_PERFORMING_RA,
  UE_WAIT_TX_ACK_MSG4,
  UE_CONNECTED
} NR_UE_L2_STATE_t;
typedef struct{
    NR_UE_L2_STATE_t state;
    //NR_CellGroupConfig_t            *cg;
    int                             servCellIndex;
   // NR_CSI_ReportConfig_t           *csirc;
    long                            physCellId;
    ////  MAC config
    int                             first_sync_frame;

    /// CRNTI
    u16 crnti;
    /// RA configuration
    RA_config_t ra;
    //UE_rar_state rar_state;
    /// SSB index from MIB decoding
    u8 mib_ssb;
    u32 mib_additional_bits;
    int mib_frame;
    /// BSR report flag management
    u8 BSR_reporting_active;
    /// LogicalChannelConfuint8_tig has bearer.
    u8 nr_band;
    u8 ssb_subcarrier_offset;
    int ssb_start_subcarrier;
}UE_MAC_INST_t;
typedef struct{
    u16 config_index;
    u16 preamble_format;
    u8 x;
    u8 y;
    u8 subframe_number[3];
    u8 start_symbol;
    u8 N_P_slots_per_subframe;
    u8 N_td_per_slot; //number of time-domain PRACH occasions within aPRACHslot
    u8 N_dur;//PRACH duration
    u8 msg1_FDM;
    u8 msg1_FrequencyStart; // low prach occasion with PRB0 offset
    u8 preambleTransMax;
    u8 powerRampingStep;
    
}prach_map_t;
typedef struct{
    u8 R;
    u16 temprnti;
    u8 ta;
    u16 ul_grant;
}RarMessage;
// MAC层RAR处理上下文

typedef struct{
    u8 currentAttempt;
    bool rarreceived;
    RarMessage receivedRAR_msg;
}UE_rar_state;
//* RAR MAC subheader // TS 38.321 ch. 6.1.5, 6.2.2 *//
// - E: The Extension field is a flag indicating if the MAC subPDU including this MAC subheader is the last MAC subPDU or not in the MAC PDU
// - T: The Type field is a flag indicating whether the MAC subheader contains a Random Access Preamble ID or a Backoff Indicator (0, BI) (1, RAPID)
// - R: Reserved bit, set to "0"
// - BI: The Backoff Indicator field identifies the overload condition in the cell.
// - RAPID: The Random Access Preamble IDentifier field identifies the transmitted Random Access Preamble

/*!\brief RAR MAC subheader with RAPID */

typedef struct {
    u32 pdu_length;
    u8* pdu;
}pdu_rar;
typedef struct {
  u8 freq_hopping;
  u8 mcs;
  u8 Msg3_t_alloc;
  u16 Msg3_f_alloc;
} UL_grant_t;
static UE_MAC_INST_t *nr_ue_mac_inst;
#endif


