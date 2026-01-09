#ifndef INFO_PROCESS_H
#define INFO_PROCESS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "globals.h"
#include "package.h"
#include "timer_queue.h"
#include "mac_process.h"
void Frame_init(frame_info **frame_p);
void RAMSG_process(msg_all *msg);
void UE_OnlineReport_process(msg_all *msg);
void UE_DataSum_process(msg_all *msg);
void Sat_DataSum_process(msg_all *msg);
u16 RNTI_to_LocalIndex_map(u32 rnti, u16 bbu_index);
u32 DIV_HEAD_Handle(type_div_head_s *div_head);
u32 DIV_MID_Handle(type_div_mid_s *div_mid);
u32 DIV_END_Handle(type_div_end_s *div_end);
void non_resident_resource_apply(type_non_resident_data_sum_s *UE_non_resident_DataSum_frame);
void sortLevel(UserInfo_Block UEinWaveTable[WAVE_NUM][PER_WAVE_MAX_NUM], u16 wave_locall_index);
void CCH_resource_release(u16 bbu_ue_index, u16 wave_local_index, u8 CCH_RB_num);
void CCH_resource_allocation(u16 ue_local_index, u16 wave_locall_index, u48 ue_addr, u32 ip, u16 wave_num, u8 CCH_RB_num, u32 rnti);
u16 wave_local_index_to_WAVE_NUM(u16 wave_locall_index);
u16 WAVE_NUM_to_LocalIndex_map(u16 wave_num);
void MAC_to_PHY_test();
void UserInfo_PtrArray_init(UserInfo **PtrArray, u32 size);
void MAC_U_init();
void online_time_table_init(int *online_time_table);
void init_RB_ID(u16 *cch_rbid, u16 sch_rbid[][RB_TOTALNUM]);
void init_CCH_idlerb_list(App_queue **ulcch_idlerb_list, App_queue **dlcch_idlerb_list, u16 *cch_rbid);
void init_SCH_idlerb_list(App_queue **ulsch_idlerb_list, App_queue **dlsch_idlerb_list, u16 sch_rbid[][RB_TOTALNUM]);
void init_DL_BusinessPackage_Queue_List(App_queue **dl_queue_list);
void init_DL_ControlPackage_Queue_List(App_queue **dl_queue_list);
void Array_init(u16 arr[], u32 size, u16 value);
void wave_ueser_mapPtrArray_init(UserInfo_Block (*PtrArray)[PER_WAVE_MAX_NUM], u32 x_size);
void per_wave_usernum_init(u8 *array, u32 size);
void UE_Data_Recv_Queue_List_init(App_queue **UE_data_recv_Queue_List);
void DL_data_buffer_Queue_List_init(App_queue **DL_data_buffer_Queue_List);
void init_DL_data_buffer_Cvector_List();
void init_RB_Reception_Info();
void init_Mac_to_PHY_data();
void UserInfo_init(UserInfo *user);
#endif