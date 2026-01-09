#ifndef LEO_RA_CONFIG_H
#define LEO_RA_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "globals.h"
#define MAX_UE 10 // 最大UE数量
extern UserInfo ueList[MAX_UE];
/*! \brief gNB template for the Random access information */
typedef struct
{
  /// Slot where preamble was received
  u8 preamble_slot;
  /// Subframe where Msg2 is to be sent
  u8 Msg2_slot;
  /// Frame where Msg2 is to be sent
  u32 Msg2_frame;
  /// Subframe where Msg3 is to be sent
  u32 Msg3_slot;
  /// Frame where Msg3 is to be sent
  u32 Msg3_frame;
  /// Msg3 time domain allocation index
  u8 Msg3_tda_id;
  /// harq_pid used for Msg4 transmission
  u8 harq_pid;
  /// UE RNTI allocated during RAR
  u16 rnti;
  /// RA RNTI allocated from received PRACH
  u16 RA_rnti;
  /// Received preamble_index
  u8 preamble_index;
  /// Received UE Contention Resolution Identifier
  u8 cont_res_id[6];
  /// Timing offset indicated by PHY
  int16_t timing_offset;
  /// Timeout for RRC connection
  int16_t RRC_timer;
  /// Msg3 first RB
  u8 msg3_first_rb;
  /// Msg3 number of RB
  u8 msg3_nb_rb;
  /// Msg3 BWP start
  u8 msg3_bwp_start;
  /// Msg3 TPC command
  u8 msg3_TPC;
  /// Msg3 ULdelay command
  u8 msg3_ULdelay;
  /// Msg3 cqireq command
  u8 msg3_cqireq;
  /// Round of Msg3 HARQ
  u8 msg3_round;
  int msg3_startsymb;
  int msg3_nrsymb;
  /// TBS used for Msg4
  int msg4_TBsize;
  /// MCS used for Msg4
  int msg4_mcs;
  /// MAC PDU length for Msg4
  int mac_pdu_length;
  /// RA search space

} NR_RA_t;
// typedef struct
// {
//   int id;               // UE标识
//   int accessAttempt;    // 随机接入尝试次数
//   int harqRetransCount; // HARQ重传计数
//   int isAccessGranted;  // 是否获得随机接入许可
// } UE;
typedef struct
{
  u8 frequency_hopping;
  u8 bwp_size;
  u8 mcs_index;
} nr_pusch_pdu_t;
void *handleRandomAccessRequest(void *arg);
void initUeList();
UserInfo *findUeById(int ueId);
void msg2_send(UserInfo *ue);
u8 *nr_fill_rar(NR_RA_t *ra, u8 *dlsch_buffer, nr_pusch_pdu_t *pusch_pdu);
int PRBalloc_to_locationandbandwidth0(int NPRB, int RBstart, int BWPsize);
u16 detectPrach();
#endif