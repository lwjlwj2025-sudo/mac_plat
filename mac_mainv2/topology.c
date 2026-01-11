#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include "topology.h"
#include "package.h"     // 需要引用 RAMSG 定义
#include "mac_process.h" // 需要引用 RAMSG_process

// 全局变量定义
BeamGeo_t g_Beam_Topology[WAVE_TOTALNUM];
UserGeo_t g_User_Topology[UE_TOTALNUM];

// 辅助：生成随机浮点数 [min, max]
double random_double(double min, double max) {
    return min + (rand() / (double)RAND_MAX) * (max - min);
}

// 辅助：计算两点距离
double calc_distance(Coordinate p1, Coordinate p2) {
    return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
}

// 1. 初始化波束拓扑 (简单的网格分布)
void Init_Topology() {
    printf("[Topology] Initializing Beam Layout (%d Beams)...\n", WAVE_TOTALNUM);
    
    // 将覆盖区域划分为网格，尽量均匀分布波束
    // 假设区域为正方形，行数和列数近似
    int cols = (int)sqrt(WAVE_TOTALNUM);
    int rows = WAVE_TOTALNUM / cols;
    if (rows * cols < WAVE_TOTALNUM) rows++;

    double step_x = COVERAGE_WIDTH / cols;
    double step_y = COVERAGE_HEIGHT / rows;

    for (int i = 0; i < WAVE_TOTALNUM; i++) {
        g_Beam_Topology[i].Wave_ID = i;
        
        // 计算行列
        int r = i / cols;
        int c = i % cols;

        // 设置波束中心坐标 (位于网格中心)
        g_Beam_Topology[i].Center.x = c * step_x + step_x / 2.0;
        g_Beam_Topology[i].Center.y = r * step_y + step_y / 2.0;
    }
}

// 2. 生成用户并进行波束匹配
void Generate_Users_And_Match() {
    printf("[Topology] Generating %d Users and Matching to Beams...\n", UE_TOTALNUM);
    srand((unsigned)time(NULL)); // 随机种子

    for (int i = 0; i < UE_TOTALNUM; i++) {
        // A. 随机生成位置
        g_User_Topology[i].Pos.x = random_double(0, COVERAGE_WIDTH);
        g_User_Topology[i].Pos.y = random_double(0, COVERAGE_HEIGHT);
        
        // B. 初始化 ID
        g_User_Topology[i].URNTI = 1000 + i; // 模拟 RNTI
        memset(g_User_Topology[i].MAC_Addr.value, 0, 6);
        g_User_Topology[i].MAC_Addr.value[5] = (u8)(i & 0xFF); // 简单模拟 MAC
        g_User_Topology[i].MAC_Addr.value[4] = (u8)((i >> 8) & 0xFF);

        // C. 随机分配业务类型 (1:Voice, 2:Video, 3:Web, 4:BG)
        //[cite_start]// 按照论文仿真设定，RT:nRT 比例约为 1:4 [cite: 183]
        int rand_val = rand() % 100;
        if (rand_val < 20) { // 20% RT
            g_User_Topology[i].Service_Type = (rand() % 2) + 1; // 1 or 2
        } else { // 80% nRT
            g_User_Topology[i].Service_Type = (rand() % 2) + 3; // 3 or 4
        }

        // D. 核心匹配逻辑：寻找最近的波束 (Matching)
        double min_dist = 999999.0;
        int best_beam = -1;

        for (int b = 0; b < WAVE_TOTALNUM; b++) {
            double d = calc_distance(g_User_Topology[i].Pos, g_Beam_Topology[b].Center);
            if (d < min_dist) {
                min_dist = d;
                best_beam = b;
            }
        }

        g_User_Topology[i].Best_Wave_ID = best_beam;
        g_User_Topology[i].Distance = min_dist;
        
        // 可选：根据距离计算初始 MCS (距离越近信号越好)
        // 这是一个简单的模拟映射
        // UserInfo 结构体中的 MCS 会在 RAMSG_process 中根据这里的逻辑进一步修正吗？
        // 实际上 RAMSG_process 应该根据信道质量来定，这里我们暂时只做位置匹配
    }
}

// // 3. 将生成的拓扑数据转化为信令注入 MAC 层
// void Inject_Users_Into_MAC() {
//     printf("[Topology] Injecting Users into MAC Layer via RAMSG...\n");

//     msg_all temp_msg;
//     // [修改] 定义一个缓冲区来模拟完整的数据包（Header + Body）
//     u8 mock_packet_buffer[sizeof(msg_head) + sizeof(type_RAMSG_s)];
//     msg_head *p_mock_head = (msg_head *)mock_packet_buffer;

//     type_RAMSG_s ramsg_payload;

//     for (int i = 0; i < UE_TOTALNUM; i++) {
        
//         // 1. 构造 RAMSG Payload 内容
//         ramsg_payload.CID = RAMSG_CID;
//         ramsg_payload.Wave_num = g_User_Topology[i].Best_Wave_ID; 
//         ramsg_payload.URNTI = g_User_Topology[i].URNTI;
//         ramsg_payload.IP = 0xC0A80000 + i; 
//         memcpy(ramsg_payload.UEMAC.value, g_User_Topology[i].MAC_Addr.value, 6);
        
//         // 填入业务参数 (用于 MLRA)
//         ramsg_payload.Service_Type = g_User_Topology[i].Service_Type;
//         ramsg_payload.Access_Type = ACCESS_TYPE_INITIAL; 

//         // 2. [修改] 填充 Mock Header
//         p_mock_head->HEAD_ID = FRAME_HEAD;
//         p_mock_head->TYPE = TYPE_RAMSG;
//         p_mock_head->LEN = sizeof(type_RAMSG_s);

//         // 3. [修改] 将 Payload 拷贝到柔性数组 DATA 的位置
//         memcpy(p_mock_head->DATA, &ramsg_payload, sizeof(type_RAMSG_s));

//         // 4. [修改] 链接到消息结构
//         temp_msg.HEAD = p_mock_head;
//         // temp_msg.TAIL = ... (如果代码中有用到 TAIL 校验，这里也需要处理，通常模拟接收可以忽略)

//         // 调用核心处理函数
//         RAMSG_process(&temp_msg);
        
//         if ((i + 1) % 500 == 0) {
//             printf("[Topology] Injected %d / %d Users...\n", i + 1, UE_TOTALNUM);
//         }
//     }
//     printf("[Topology] Injection Complete.\n");
// }
// 修改 Inject_Users_Into_MAC 函数
void Inject_Users_Into_MAC() {
    printf("[Topology] Injecting Users into RACH Queue...\n");

    type_RAMSG_s ramsg_payload;

    for (int i = 0; i < UE_TOTALNUM; i++) {
        // ... (构造 ramsg_payload 的代码保持不变) ...
        ramsg_payload.CID = RAMSG_CID;
        ramsg_payload.Wave_num = g_User_Topology[i].Best_Wave_ID;
        ramsg_payload.URNTI = g_User_Topology[i].URNTI;
        ramsg_payload.IP = 0xC0A80000 + i;
        memcpy(ramsg_payload.UEMAC.value, g_User_Topology[i].MAC_Addr.value, 6);
        ramsg_payload.Service_Type = g_User_Topology[i].Service_Type;
        ramsg_payload.Access_Type = ACCESS_TYPE_INITIAL;

        // [MODIFIED] 将请求推入全局 RACH 队列，而不是直接调用函数
        // 这样 Thread_RACH_Process 会在后台慢慢处理它们
        if(App_queue_push(&Queue_RACH_Req, (u8*)&ramsg_payload, sizeof(type_RAMSG_s)) != QUEUE_OK) {
            printf("[Topology] Warning: RACH Queue Full at user %d!\n", i);
            break; // 队列满则停止注入
        }
        
        if ((i + 1) % 500 == 0) {
            printf("[Topology] Queued %d / %d Users...\n", i + 1, UE_TOTALNUM);
        }
    }
    printf("[Topology] Injection to Queue Complete.\n");
}
// 全局变量
u8 g_Interference_Matrix[WAVE_TOTALNUM][WAVE_TOTALNUM];

// 初始化干扰矩阵 (在 Init_Topology 后调用)
void Init_Interference_Matrix() {
    printf("[Topology] Building Beam Interference Matrix (Threshold: %.2f km)...\n", INTERFERENCE_TH);
    
    for (int i = 0; i < WAVE_TOTALNUM; i++) {
        for (int j = 0; j < WAVE_TOTALNUM; j++) {
            if (i == j) {
                g_Interference_Matrix[i][j] = 1; // 自己对自己算“占用”
                continue;
            }
            
            // 计算两波束中心距离
            double dist = calc_distance(g_Beam_Topology[i].Center, g_Beam_Topology[j].Center);
            
            // 依据论文公式 (4): d(i,j) <= 2R 则置 1
            if (dist <= INTERFERENCE_TH) {
                g_Interference_Matrix[i][j] = 1;
            } else {
                g_Interference_Matrix[i][j] = 0;
            }
        }
    }
}

// 获取用户到波束中心的距离 (用于计算 L_j)
double Get_User_Distance_To_Center(u16 wave_id, Coordinate user_pos) {
    if(wave_id >= WAVE_TOTALNUM) return 9999.0;
    return calc_distance(g_Beam_Topology[wave_id].Center, user_pos);
}