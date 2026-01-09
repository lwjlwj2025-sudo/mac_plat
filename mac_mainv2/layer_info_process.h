#ifndef LAYER_INFO_PROCESS_H
#define LAYER_INFO_PROCESS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "globals.h"
#define rlc_max 500               // recv from rrc data maxlen
extern u8 recv_rlc_data[rlc_max]; // recv from rrc data
extern u32 recv_rlc_data_len;     // recv from rrc datalen
typedef enum
{
    data_send = 0,
    status_report = 1,
    tx_op = 2
} RLC_Info_type;
/*现在从RLC来的数据结构是这样的*/
typedef struct
{
    RLC_Info_type rlcMsgType; // 0
    u8 ueID;
    u8 TranChanID : 4;
    u8 LogChanID : 4; // choose which logicalchannel
    u8 DATA[0];
} RLC_MAC_data;
typedef struct
{
    int Frame;
    int UeID;
    u8 PhyMsgType : 4;
    u8 TranChanID : 4;
    u8 DATA[0];
} MAC_PHY_data; // info
void *RecvPackage_from_Sat_RRC(void *arg);
void *RecvPackage_from_UE_MAC_U(void *arg);
void *RecvPackage_from_Sat_RLC(void *arg);
#endif