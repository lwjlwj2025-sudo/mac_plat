#ifndef SCHEDULING_H
#define SCHEDULING_H
#include "globals.h"
#include "package.h"
#include "timer_queue.h"
void prepare_next_subframe_send_data();
void poll_wave_index_renew();
void SCH_RB_Reception_info_renew();
u16 CCH_pk_generate(u16 ue_local_index, u16 ue_index);
u32 Mac_PHY_rec_frame_Hendler(u8 user_num);
u32 Mac_PHY_send_frame_Hendler(u8 user_num);
double Calculate_Jains_Fairness() ;
#endif