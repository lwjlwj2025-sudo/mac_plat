#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "CONSTANTS.h"
#include "globals.h"
// 区域范围 (km)
#define COVERAGE_WIDTH  1000.0
#define COVERAGE_HEIGHT 1000.0

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
// 地理坐标结构
typedef struct {
    double x;
    double y;
} Coordinate;

// 波束地理信息
typedef struct {
    u16 Wave_ID;        // 虚波位 ID (0 ~ 599)
    Coordinate Center;  // 波束中心坐标
} BeamGeo_t;

// 用户地理信息
typedef struct {
    u32 URNTI;          // 临时 ID
    u48 MAC_Addr;       // 物理地址
    Coordinate Pos;     // 用户坐标
    u16 Best_Wave_ID;   // 匹配到的最佳波束 ID
    u8 Service_Type;    // 业务类型 (用于 MLRA)
    double Distance;    // 距离波束中心的距离 (用于计算信号质量/MCS)
} UserGeo_t;

// 全局拓扑数据
extern BeamGeo_t g_Beam_Topology[WAVE_TOTALNUM];
extern UserGeo_t g_User_Topology[UE_TOTALNUM];
// 1 表示两波束距离 < 2R (存在干扰)，0 表示无干扰
extern u8 g_Interference_Matrix[WAVE_TOTALNUM][WAVE_TOTALNUM];

// 函数声明
void Init_Interference_Matrix();
double Get_User_Distance_To_Center(u16 wave_id, Coordinate user_pos); // 计算用户到波束中心距离
// 函数声明
void Init_Topology();
void Generate_Users_And_Match();
void Inject_Users_Into_MAC();

#endif