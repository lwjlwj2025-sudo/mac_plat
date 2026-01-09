#ifndef CONSTANTS_H
#define CONSTANTS_H

#define MY_MAC_ADDR "00:00:00:00:00:02"
#define MY_MAC_ADDR_BYTE_NUM 6

// CID
#define MIB1_CID 0xFE
#define MIB2_CID 0xFD
#define RAREQ_CID 0x01
#define RAMSG_CID 0x02
#define RARSP_CID 0x03
#define RACOM_CID 0x04
#define SSR_CID 0x05
#define USR_CID 0x06
#define RRC_Release_CID 0x07
#define Disconnect_MSG_CID 0x08
#define UE_OnlineReport_CID 0x09
#define Sat_OnlineReport_CID 0x0A
#define DATA_SUM_CID 0x0B
#define SCH_FILE_CID 0x11

#define BBU_UE_MAXNUM 250
#define DIV_DATA_LEN 40
#define SCH_SLOT_NUM 3

// 拆包可容纳数据大小 第一包 32B 中间包 33B 尾包 不定长
#define HEAD_DATA_LEN 32
#define MID_DATA_LEN 33

// 发送给物理层带车皮的包长度
#define TO_PHY_DATA_LEN 45

// 单位RB传输业务量（字节）

#define L_RB 40

// 八种优先级业务权重占比，优先级越高，所占比重越大

#define Q1 0.2125
#define Q2 0.1875
#define Q3 0.1625
#define Q4 0.1375
#define Q5 0.1125
#define Q6 0.0875
#define Q7 0.0625
#define Q8 0.0375

// 三种用户优先级权重
#define P1 0.5
#define P2 0.3
#define P3 0.2

// 业务包判别头
#define PK_FRAMEHEAD 0x6666
#define CCH_FRAMEHEAD 0x7777
#define SCH_FRAMEHEAD 0x8888

// 时间和周期
// 微秒
#define guard_time 30       // 单位：微秒    窄波束子帧头保护时间
#define frame_length 100000 // 单位：微秒	窄波束帧长度100ms
#define subframe_length 500 // 单位：微秒    窄波束子帧长度0.5ms
#define slot_length 123.76  // 单位：微秒	窄波束时隙长度 123.76us
// 秒
#define slot_div_intrpt_period 0.001
#define slot_per_frame 800
// 时隙类型标签
#define MIB1_SLOT 201
#define MIB2_SLOT 202
#define RES_SLOT 203
#define GT_SLOT 255
#define CCH_SLOT 204
#define SCH_SLOT 205
// [MLRA] 算法参数
#define MLRA_OMEGA 0.6         // 权重因子 omega (论文中用于平衡公平性和效率)
#define MLRA_TAU_RT 50         // 实时业务最大容忍时延 (ms)
#define MLRA_TAU_NRT 200       // 非实时业务最大容忍时延 (ms)
#define MLRA_PLOSS_RT 0.01     // 实时业务允许丢包率 1%
#define MLRA_ALPHA_RT 0.04     // a_j = -log(Ploss)/Tau ≈ -log(0.01)/50 ≈ 0.09 (简化取值)

// 波束调度窗口
#define BEAM_SELECT_TOP_K 1    // 每次调度选择负载最高的 K 个波束 (单BBU通常一次处理一个波位)
// [NEW] 业务类型定义 (Service Types)
#define SERVICE_VOICE       0x01 // 语音 (高优先级)
#define SERVICE_VIDEO       0x02 // 视频 (中高优先级)
#define SERVICE_WEB         0x03 // 网页 (中优先级)
#define SERVICE_BACKGROUND  0x04 // 后台下载 (低优先级)

// [NEW] 接入类型定义 (Access Types)
#define ACCESS_TYPE_INITIAL    0x01 // 初始连接
#define ACCESS_TYPE_SCHEDULING 0x02 // 已有连接，请求调度

#endif