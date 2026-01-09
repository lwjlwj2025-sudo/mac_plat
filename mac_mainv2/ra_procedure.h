#ifndef RA_PROCEDURE_H
#define RA_PROCEDURE_H
#include "ra_config.h"
//static UE_MAC_INST_t *nr_ue_mac_inst;
void init_RA(RA_config_t *ra,PRACH_RESOURCES_t *prach_resources);
u16 get_DELTA_PREAMBLE(u16 prach_format);
u16 get_power_prach(RA_config_t *ra);
u16 get_ra_rnti(RA_config_t *ra);
void get_prach_resources(module_id_t mod_id);
static const u16 SSB_RSRP[4]={2,1,4,8};  //from phy
u16 get_max_SSB_RSRP(RACH_Config_rrc * rrc_config);
u8 msg1_transmitted(RA_config_t *ra , u16 *frame,prach_map_t *prachmap);

void* randomaccessProcedure(void* arg);
u16 prach_occasion_config(prach_map_t *prachmap,u8 *l,u16 *occasion_subframe_num,u16 *occasion_frame_num);
u16 ssb_to_ro_list(RA_config_t *ra , u8 *l , u16 MAX_SSB_index,prach_map_t *prachmap);
int ZC_preamble(float zc_sequence[][PREAMBLE_LENGTH]);
int send_preamble();
u16 handle_rar(MacRarContext *mac_rar_ctx,RA_config_t *ra);
void listenRARWindow(RA_config_t *ra,MacRarContext *mac_rar_ctx);
MacRarContext* get_rar();
#endif
