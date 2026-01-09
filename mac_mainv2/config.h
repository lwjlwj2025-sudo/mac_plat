#ifndef CONFIG_H
#define CONFIG_H

#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <pthread.h>
#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "CONSTANTS.h"
// #include "scheduling.h"
//  #include "ra_config.h"

#define scs1 120 // khz
#define OFDM_per_slot 14
#define MAX_RAR_BUFFER 100 // RAR消息的最大缓冲区大小
#define subframe_NUM (frame_length / subframe_length)
#define MAX_PRACH_OCCASIONS 16 // 最大PRACH时隙数量
#define filename(x) strrchr(x, '/') ? strrchr(x, '/') + 1 : x
#define LOG(format, args...) (printf("[%s]-[%d]:" format "\r\n", filename(__FILE__), __LINE__, ##args))
typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned short u16;
extern pthread_mutex_t lock;
extern pthread_cond_t cond1;
extern pthread_cond_t cond2;
typedef struct NR_UL_TIME_ALIGNMENT
{
  /// TA command and TAGID received from the gNB
  bool ta_apply;
  int ta_command;
  int ta_total;
  u32 tag_id;
  int frame;
  int slot;
} NR_UL_TIME_ALIGNMENT_t; // table 7.2-1 TS 38.321
typedef struct
{
  int index; // PRACH资源索引
  // int signalStrength;   // 信号强度
  int isOccupied; // 是否被占用
} PRACHResource;
// 基站的PRACH资源列表

extern PRACHResource prachResources[];

typedef struct
{
  u16 rar_buffer_size;           // 当前RAR缓冲区大小
  u8 rar_buffer[MAX_RAR_BUFFER]; // RAR缓冲区

  // u8 backoff_slot;               // 当前退避时隙
} MacRarContext;
typedef struct
{
  uint8_t RAPID : 6;
  uint8_t T : 1;
  uint8_t E : 1;
} NR_RA_HEADER_RAPID;

/*!\brief RAR MAC subheader with Backoff Indicator */
typedef struct
{
  uint8_t BI : 4;
  uint8_t R : 2;
  uint8_t T : 1;
  uint8_t E : 1;
} NR_RA_HEADER_BI; // E shi gao wei

// TS 38.321 ch. 6.2.3
typedef struct
{

  uint8_t TA1 : 7;        // octet 1 [6:0]
  uint8_t R : 1;          // octet 1 [7] gaowei
  uint8_t UL_GRANT_1 : 3; // octet 2 [2:0]
  uint8_t TA2 : 5;        // octet 2 [7:3] gaowei
  uint8_t UL_GRANT_2 : 8; // octet 3 [7:0]
  uint8_t UL_GRANT_3 : 8; // octet 4 [7:0]
  uint8_t UL_GRANT_4 : 8; // octet 5 [7:0]
  uint8_t TCRNTI_1 : 8;   // octet 6 [7:0]
  uint8_t TCRNTI_2 : 8;   // octet 7 [7:0]
} NR_MAC_RAR;
typedef struct
{
  u8 LCID;                    // 逻辑信道标识
  u8 LCGID;                   // 逻辑信道组标识
  u8 Bj;                      // 令牌桶中的令牌数量
  u8 PBR;                     // 令牌填充速率 kbps
  u8 BSD;                     // 令牌桶最大容量 ms
  u8 priority;                // 优先级
  u8 waitingdatabuffer;       // 等待发送的数据量
  bool logicalchannelSR_mask; // SR触发条件
  u8 PDUnum;
  u8 PDUsize[4];
  u8 status; //
} Logicalchannel;

typedef struct
{
  u8 ULGrant; // 上行授权资源量
} UplinkGrant;
typedef struct
{
  u8 RBG;
  u8 RB;
  u8 L;
  u8 S;
  u16 per_RB_bytenum;
} DCI_TB_info;
typedef struct
{
  Logicalchannel logicalchannel[3]; // 最多8个逻辑信道
  UplinkGrant uplinkGrant;          // 上行授权
  u8 numChannels;                   // 逻辑信道数量
  u8 TB;                            // 得有一个分配得到的RB存储
  DCI_TB_info UE_TB_info;
  u8 BSR[2];
} ue_schedule;
extern MacRarContext mac_rar_ctx;
#endif