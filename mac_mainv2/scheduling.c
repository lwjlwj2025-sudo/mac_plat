#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "CONSTANTS.h"
#include "mac_process.h"
#include "globals.h"
#include "package.h"
// #include    "App_queue.h"
#include "App_udp.h"
#include "scheduling.h"
#include "info_process.h"
#include "config.h"

/*
准备下一轮要给物理层的数据
*/
void prepare_next_subframe_send_data()
{
    pthread_mutex_lock(&Global_MAC_Lock);
    // 虚波位轮询
    //poll_wave_index_renew();
    poll_wave_index_renew_MLRA();
    sortLevel_MLRA(wave_ueser_mapPtrArray, poll_wave_index);
    /*
    用户注册列表中记录相邻两个虚波位的上行业务资源信息，早一些的是接下来要接收的用户发来的业务所在的资源信息，
    也就是用户已经收到的业务资源信息；最新的是将要通知用户的上行业务资源信息。

    这一步必须在每个子帧最开始的位置完成, 也就是需要在用户的上行业务资源申请发来之前完成业务资源的更新，或者是在本次
    */
    SCH_RB_Reception_info_renew(); // 业务更新

    // 用户每个RB可以承载的字节数
    u16 per_RB_ByteNum;
    // 单个用户分到的连续RB的开始和结尾，以及分到的RB的总数
    u16 user_RB_begin, user_RB_TotalNum;
    // 某个用户的所有即将发送实际业务（未补零）的总量
    u32 user_send_total_data;
    // 剩余可用的SCH_RB
    u16 surplus_SCH_RB;
    // 剩余可用的字节数总数。与用户可以发送的字节总数1024B不同
    u16 surplus_ByteNum;
    u16 ue_local_index;
    u8 send_user_num; // 记录本轮需要发出多上个用户的数据

    // 初始化非驻留业务可用RB起点
    DL_min_flash_available_idleRB = DL_min_idleRB;
    // 初始化要发送数据的用户数
    send_user_num = 0;

    u16 ue_index;
    ue_index = 0;
    for (ue_index; ue_index < PER_WAVE_MAX_NUM; ue_index++)
    {
        UserInfo_Block *ue = &wave_ueser_mapPtrArray[poll_wave_index][ue_index];
        if (ue->State == 0) break;

        
        // if(poll_wave_index != 100)
        // {
        //     break;
        // }
        // LOG("wave_ueser_mapPtrArray[poll_wave_index][ue_index].State = %d\n", wave_ueser_mapPtrArray[poll_wave_index][ue_index].State);
        // LOG("User_DataQueue_ptr = %x\n", wave_ueser_mapPtrArray[poll_wave_index][ue_index].User_DataQueue_ptr);
        // LOG("wave_locall_index = %d   index = %d\n", poll_wave_index, ue_index);

        if (wave_ueser_mapPtrArray[poll_wave_index][ue_index].State == 0) // 在这加随机接入,随机接入成功后，state置为1
        {
            // 因为用户是按接入顺序以此接入的，则这个位置没有用户接入，后续位置都没有用户接入
            //  if(poll_wave_index == 100)
            //      LOG("当前虚波位没有用户接入\n");
            break;
        }
        // 检测到存在接入的用户，累加需要发送数据的用户数
        send_user_num++;

        if (wave_ueser_mapPtrArray[poll_wave_index][ue_index].User_DataQueue_ptr->size == 0)
        {
            // 如果这个用户没有缓存的待发送数据，则跳过这个用户，为下一个用户分配资源
            // LOG("当前用户没有业务资源\n");
            continue;
        }

        LOG("开始为某个用户分配业务资源\n");

        // 计算当前处理的用户的每个RB可以承载的字节数
        per_RB_ByteNum = cal_RB_carry_capacity(wave_ueser_mapPtrArray[poll_wave_index][ue_index].MCS); // 根据调制方式
        LOG("MCS = %d    per_RB_ByteNum = %d", wave_ueser_mapPtrArray[poll_wave_index][ue_index].MCS, per_RB_ByteNum);
        // 计算当前可用的RB数量能够支持该用户发送多少字节的数据，现在业务RB_ID改到了250～999，SCH_RB_NUM + RB_TOTALNUM = 1000
        surplus_SCH_RB = SCH_RB_NUM + RB_TOTALNUM - DL_min_flash_available_idleRB;
        LOG("DL_min_flash_available_idleRB = %d\n", DL_min_flash_available_idleRB);
        surplus_ByteNum = surplus_SCH_RB * per_RB_ByteNum;

        user_RB_begin = DL_min_flash_available_idleRB;
        user_RB_TotalNum = 0;
        user_send_total_data = 0;

        ue_local_index = RNTI_to_LocalIndex_map(wave_ueser_mapPtrArray[poll_wave_index][ue_index].U_RNTI, BBU_index);

        while (wave_ueser_mapPtrArray[poll_wave_index][ue_index].User_DataQueue_ptr->size > 0)
        {
            LOG("wave_ueser_mapPtrArray[poll_wave_index][ue_index].User_DataQueue_ptr->size = %u\n", wave_ueser_mapPtrArray[poll_wave_index][ue_index].User_DataQueue_ptr->size);
            // 获取队列头部业务数据包的大小
            u32 data_len = wave_ueser_mapPtrArray[poll_wave_index][ue_index].User_DataQueue_ptr->head->data_len;

            // 判断这个包是否会使将要发出去的业务信道数据超出 PER_UE_MAX_DL_VOLUME
            user_send_total_data += data_len;
            if (user_send_total_data > (PER_UE_MAX_DL_VOLUME - MAC_FRAME_LENGTH))
            {
                LOG("超出单个用户单次最多可发送的业务数据的大小\n");
                user_send_total_data -= data_len;
                // 该用户单次可发送业务余量耗尽，去为下一个用户分配资源
                break;
            }
            // 判断剩余RB可供使用的字节数够不够用
            if (surplus_ByteNum < data_len)
            {
                LOG("空闲SCH_RB提供的字节数承载能力不足\n");
                user_send_total_data -= data_len;
                break;
            }
            surplus_ByteNum -= data_len;

            // 这个包可以发出
            // 从vector中取出包
            char data[1024] = {0};
            cvector_pophead(wave_ueser_mapPtrArray[poll_wave_index][ue_index].User_DataQueue_ptr, data, data_len); // 取出头部数据的长度
            App_queue_push(&Queue_DL_SCH_DATA_Packaging_Buffer, data, data_len);
        }
        user_RB_TotalNum = ceil((double)(user_send_total_data + MAC_FRAME_LENGTH) / per_RB_ByteNum);
        surplus_SCH_RB -= user_RB_TotalNum;
        DL_min_flash_available_idleRB += user_RB_TotalNum;
        // 将分配给用户的下行业务资源信息填入用户注册列表
        UserInfo_PtrArray[ue_local_index]->DL_SCH_RB_begin = user_RB_begin;
        UserInfo_PtrArray[ue_local_index]->DL_SCH_RB_Total_NUM = user_RB_TotalNum;

        // 在结构体里填写内容Mac-PHY 发射端
        Mac_to_PHY_data[ue_index].CCH_RB_begin = UserInfo_PtrArray[ue_local_index]->CCH_FrequencyPoint.slot0_downlink_FrequencyPoint;
        // CCH的MAC头
        Mac_to_PHY_data[ue_index].CCH_frame_head = CCH_FRAMEHEAD;
        memcpy(Mac_to_PHY_data[ue_index].CCH_DEST_MAC.value, UserInfo_PtrArray[ue_local_index]->UE_MAC_addr.value, MY_MAC_ADDR_BYTE_NUM);
        memcpy(Mac_to_PHY_data[ue_index].CCH_SRC_MAC.value, own_address.value, MY_MAC_ADDR_BYTE_NUM);
        Mac_to_PHY_data[ue_index].CCH_Data_Length = CCH_DATA_LENGTH - MAC_FRAME_LENGTH;

        Mac_to_PHY_data[ue_index].SCH_MCS = UserInfo_PtrArray[ue_local_index]->DL_MCS;
        // 传给物理层的业务信道数据总量，也就是传给物理层大包中的SCH_DATA的长度，因为不满一个RB则要补零
        Mac_to_PHY_data[ue_index].SCH_TBS = user_RB_TotalNum * per_RB_ByteNum;
        Mac_to_PHY_data[ue_index].SCH_RB_Begin = user_RB_begin;
        Mac_to_PHY_data[ue_index].SCH_RB_Length = user_RB_TotalNum;
        LOG("SCH_TBS = %d\n", Mac_to_PHY_data[ue_index].SCH_TBS);
        LOG("SCH_RB_Begin = %d\n", Mac_to_PHY_data[ue_index].SCH_RB_Begin);
        LOG("SCH_RB_Length = %d\n", Mac_to_PHY_data[ue_index].SCH_RB_Length);
        // SCH的MAC头
        Mac_to_PHY_data[ue_index].SCH_frame_head = SCH_FRAMEHEAD;
        memcpy(Mac_to_PHY_data[ue_index].SCH_DEST_MAC.value, UserInfo_PtrArray[ue_local_index]->UE_MAC_addr.value, MY_MAC_ADDR_BYTE_NUM);
        memcpy(Mac_to_PHY_data[ue_index].SCH_SRC_MAC.value, own_address.value, MY_MAC_ADDR_BYTE_NUM);
        Mac_to_PHY_data[ue_index].SCH_Data_Length = user_RB_TotalNum * per_RB_ByteNum - MAC_FRAME_LENGTH;

        // 将SCH数据补零后压入结构体压入组帧用的结构体中
        char SCH_buffer[2000] = {0};
        App_queue_pop(&Queue_DL_SCH_DATA_Packaging_Buffer, SCH_buffer, user_send_total_data, 0x03);
        // 多的一步内存拷贝相当于给只用了半个的RB补零
        memcpy(Mac_to_PHY_data[ue_index].SCH_data, SCH_buffer, Mac_to_PHY_data[ue_index].SCH_TBS);
        App_queue_init(&Queue_DL_SCH_DATA_Packaging_Buffer);

        // 生成控制信令，并压入Mac-PHY 发射端结构体
        u16 CCH_data_length = CCH_pk_generate(ue_local_index, ue_index);

        // Mac-PHY 接收端
        RB_Reception_Info[ue_index].CCH_RB_begin = UserInfo_PtrArray[ue_local_index]->CCH_FrequencyPoint.slot0_uplink_FrequencyPoint;
        RB_Reception_Info[ue_index].SCH_MCS = UserInfo_PtrArray[ue_local_index]->UL_MCS;
        RB_Reception_Info[ue_index].SCH_TBS = UserInfo_PtrArray[ue_local_index]->eatrly_UL_SCH_RB_Total_NUM * cal_RB_carry_capacity(UserInfo_PtrArray[ue_local_index]->UL_MCS);
        RB_Reception_Info[ue_index].SCH_RB_Begin = UserInfo_PtrArray[ue_local_index]->eatrly_UL_SCH_RB_begin;
        RB_Reception_Info[ue_index].SCH_RB_Length = UserInfo_PtrArray[ue_local_index]->eatrly_UL_SCH_RB_Total_NUM;
        u32 allocated_bytes = user_send_total_data;
        // LOG("发给用户 %d控制信令与业务数prepare_next_subframe_send_data据准备完成！\n", UserInfo_PtrArray[ue_local_index]->UE_MAC_addr);
        update_average_rate(ue, allocated_bytes);
        
        // [MLRA] 更新时延：如果有积压且未完全发完，时延增加；发完则清零
        if (ue->User_DataQueue_ptr->size > 0) {
            ue->Head_Of_Line_Delay += 1; // 假设每调度周期 1ms
        } else {
            ue->Head_Of_Line_Delay = 0;
        }
        // 假设本轮分配了 allocated_bytes
        if (allocated_bytes > 0) {
            // 更新统计
            g_MacStats.total_throughput_bytes += allocated_bytes;
            g_MacStats.total_rb_used += user_RB_TotalNum;
            if (ue->Is_RealTime) 
                g_MacStats.rt_throughput_bytes += allocated_bytes;
            else 
                g_MacStats.nrt_throughput_bytes += allocated_bytes;

            // [LOG] 打印调度详情：体现优先级起作用了
            // 格式：[Sched] UEID | 类型 | 优先级 | 队列 | 历史速率 | 分配RB
            LOG("[Sched-UE] ID:%d | %s | Prio:%.2f | Q:%d | AvgR:%.1f | Alloc:%d RB\n", 
                ue->U_RNTI, 
                (ue->Is_RealTime ? "RT " : "nRT"), 
                ue->Dynamic_Priority, 
                ue->User_DataQueue_ptr->size,
                ue->Avg_Data_Rate,
                user_RB_TotalNum);
        }
    }
    // 统计总资源
    g_MacStats.total_rb_available += SCH_RB_NUM;
    // 将结构体数组Mac_to_PHY中所有数据整合到一个BUFFER中，等待时机发送出去
    if (send_user_num != 0)
    {
        u32 rec_frame_len = Mac_PHY_rec_frame_Hendler(send_user_num);
        u32 send_frame_len = Mac_PHY_send_frame_Hendler(send_user_num);

        // 测试用
        /*u8 arr[100];
        int index;
        index = 0;
        for(index; index < 100; index++)
        {
            arr[index] = index;
        }
        pcie_write_to_fpga(arr, 100, PCIE_WRITE1_SFFSET);
        //pcie_write_to_fpga(Mac_to_PHY_send_frame_buffer, send_frame_len, PCIE_WRITE1_SFFSET);
        pcie_write_to_fpga(Mac_to_PHY_rec_frame_buffer, rec_frame_len, PCIE_WRITE2_SFFSET);*/
    }
    // [NEW] 调度结束解锁
    pthread_mutex_unlock(&Global_MAC_Lock);
}
// 虚波位轮询的虚波位更新
void poll_wave_index_renew()
{
    if (poll_wave_index < 149)
    {
        poll_wave_index++;
    }
    else if (poll_wave_index == 149)
    {
        poll_wave_index = 0;
    }
    else
    {
        LOG("poll_wave_index error");
    }
}
/*
更新本轮需要接收的SCH_RB信息
*/
void SCH_RB_Reception_info_renew()
{

    u8 ue_index;
    u16 ue_local_index;

    for (ue_index; ue_index < PER_WAVE_MAX_NUM; ue_index++)
    {
        if (wave_ueser_mapPtrArray[poll_wave_index][ue_index].State == 0)
        {
            break;
        }
        ue_local_index = RNTI_to_LocalIndex_map(wave_ueser_mapPtrArray[poll_wave_index][ue_index].U_RNTI, BBU_index);

        UserInfo_PtrArray[ue_local_index]->eatrly_UL_SCH_RB_begin = UserInfo_PtrArray[ue_local_index]->UL_SCH_RB_begin;
        UserInfo_PtrArray[ue_local_index]->eatrly_UL_SCH_RB_Total_NUM = UserInfo_PtrArray[ue_local_index]->UL_SCH_RB_Total_NUM;
        UserInfo_PtrArray[ue_local_index]->UL_SCH_RB_begin = 0xFFFF;
        UserInfo_PtrArray[ue_local_index]->UL_SCH_RB_Total_NUM = 0xFFFF;
    }
}
// 生成下行控制信令数据包
// 入参：buffer：发送到物理层的帧的控制信道数据, ue_local_index：用户在基带板上的索引;ue_index: 用户在虚波位上的索引
u16 CCH_pk_generate(u16 ue_local_index, u16 ue_index)
{
    u16 FrameLength;
    u16 CCH_Data_Length;
    char buffer[64] = {0};
    char frame_buffer[128] = {0};

    CCH_Data_Length = 0;

    // 生成卫星在线状态报告
    type_Sat_OnlineReport_s *sat_online_report = (type_Sat_OnlineReport_s *)buffer;
    sat_online_report->CID = TYPE_SAT_DATA_SUM;
    sat_online_report->wave_num = poll_wave_index;
    // 加解大包包用的帧头
    FrameLength = SCH_DATA_Packaging(buffer, sizeof(type_Sat_OnlineReport_s), frame_buffer);
    // 压入组帧buffer
    App_queue_push(&Queue_DL_CCH_DATA_Packaging_Buffer, frame_buffer, FrameLength);
    CCH_Data_Length += FrameLength;
    LOG("MACU_process：卫星在线状态报告生成！\n");

    // 生成上行业务资源信息包
    if (UserInfo_PtrArray[ue_local_index]->UL_SCH_RB_begin != 0xFFFF)
    {
        type_non_resident_SCH_FILE_s *UL_non_resident_SCH_FILE = (type_non_resident_SCH_FILE_s *)buffer;
        UL_non_resident_SCH_FILE->U_or_D = 0;
        UL_non_resident_SCH_FILE->RB_begin = UserInfo_PtrArray[ue_local_index]->UL_SCH_RB_begin;
        UL_non_resident_SCH_FILE->RB_TotalNum = UserInfo_PtrArray[ue_local_index]->UL_SCH_RB_Total_NUM;

        // 加解大包包用的帧头
        FrameLength = SCH_DATA_Packaging(buffer, sizeof(type_non_resident_SCH_FILE_s), frame_buffer);
        // 压入组帧buffer
        App_queue_push(&Queue_DL_CCH_DATA_Packaging_Buffer, frame_buffer, FrameLength);
        CCH_Data_Length += FrameLength;
        LOG("MACU_process：上行业务资源信息包生成！\n");
    }

    // 生成下行业务资源信息包
    if (UserInfo_PtrArray[ue_local_index]->DL_SCH_RB_begin != 0xFFFF)
    {
        type_non_resident_SCH_FILE_s *UL_non_resident_SCH_FILE = (type_non_resident_SCH_FILE_s *)buffer;
        UL_non_resident_SCH_FILE->U_or_D = 1;
        UL_non_resident_SCH_FILE->RB_begin = UserInfo_PtrArray[ue_local_index]->DL_SCH_RB_begin;
        UL_non_resident_SCH_FILE->RB_TotalNum = UserInfo_PtrArray[ue_local_index]->DL_SCH_RB_Total_NUM;

        // 加解大包包用的帧头
        FrameLength = SCH_DATA_Packaging(buffer, sizeof(type_non_resident_SCH_FILE_s), frame_buffer);
        // 压入组帧buffer
        App_queue_push(&Queue_DL_CCH_DATA_Packaging_Buffer, frame_buffer, FrameLength);
        CCH_Data_Length += FrameLength;
        LOG("MACU_process：下行业务资源信息包生成！\n");
    }

    // 生成切换信令

    // 信令生成完毕，将数据压入
    App_queue_pop(&Queue_DL_CCH_DATA_Packaging_Buffer, Mac_to_PHY_data[ue_index].CCH_DATA, CCH_Data_Length, 0x03);
    // 未使用的位置都置零，防止上一子帧的控制信令干扰本子帧的控制信令, -14是减去MAC帧头
    memset(&Mac_to_PHY_data[ue_index].CCH_DATA[CCH_Data_Length], 0, CCH_DATA_LENGTH - MAC_FRAME_LENGTH - CCH_Data_Length);
    // App_queue_pop(&Queue_DL_CCH_DATA_Packaging_Buffer, cch_data_buffer, CCH_Data_Length);
    App_queue_init(&Queue_DL_CCH_DATA_Packaging_Buffer);
    LOG("MACU_process：下行控制信令生成！\n");
    return CCH_Data_Length;
}
/*
Mac-PHY 接收端组帧，告知物理层要发送的数据以及相应的信道资源信息
*/
u32 Mac_PHY_rec_frame_Hendler(u8 user_num)
{
    memcpy(Mac_to_PHY_rec_frame_buffer, &user_num, 1);

    u32 CCH_offset;
    u32 SCH_offset;
    u32 frame_len;

    frame_len = 1 + user_num * 8;

    CCH_offset = 1;
    SCH_offset = 1 + user_num * 1;
    u8 index;
    index = 0;
    for (index; index < user_num; index++)
    {
        memcpy(&Mac_to_PHY_rec_frame_buffer[CCH_offset], &RB_Reception_Info[index], 1);
        CCH_offset += 1;

        memcpy(&Mac_to_PHY_rec_frame_buffer[SCH_offset], &RB_Reception_Info[index].SCH_MCS, 7);
        SCH_offset += 7;
    }
    LOG("MACU_process: Mac-PHY 接收端组帧组帧完成!\n");
    return frame_len;
}
/*
Mac-PHY 发射端组帧，告知物理层要发送的数据以及相应的信道资源信息
*/
u32 Mac_PHY_send_frame_Hendler(u8 user_num)
{
    memcpy(Mac_to_PHY_send_frame_buffer, &user_num, 1);

    u32 CCH_offset;
    u32 SCH_offset;
    u32 frame_len;

    frame_len = 1; // 用户数user_num始终都会占用1B，无论是否有用户

    CCH_offset = 1;
    SCH_offset = 1 + user_num * (1 + CCH_DATA_LENGTH);
    u8 index;
    index = 0;
    for (index; index < user_num; index++)
    {
        memcpy(&Mac_to_PHY_send_frame_buffer[CCH_offset], &Mac_to_PHY_data[index].CCH_RB_begin, 1 + CCH_DATA_LENGTH);
        CCH_offset += (1 + CCH_DATA_LENGTH);

        memcpy(&Mac_to_PHY_send_frame_buffer[SCH_offset], &Mac_to_PHY_data[index].SCH_MCS, 7 + Mac_to_PHY_data[index].SCH_TBS);

        // 测试代码
        // u16 SCH_RB_begin = 0;
        // memcpy(&SCH_RB_begin, &Mac_to_PHY_send_frame_buffer[SCH_offset + 1 + 2], 2);
        // LOG("user %d  SCH_RB_begin = %d\n", index, SCH_RB_begin);
        // u16 SCH_RB_total = 0;
        // memcpy(&SCH_RB_total, &Mac_to_PHY_send_frame_buffer[SCH_offset + 1 + 2 + 2], 2);
        // LOG("user %d  SCH_RB_total = %d\n", index, SCH_RB_total);

        SCH_offset += (7 + Mac_to_PHY_data[index].SCH_TBS);

        frame_len += (1 + CCH_DATA_LENGTH + 7 + Mac_to_PHY_data[index].SCH_TBS);
    }
    LOG("MACU_process: Mac-PHY 发射端组帧组帧组帧完成!\n");
    // LOG("user_num = %d\n", Mac_to_PHY_send_frame_buffer[0]);

    // testSatFramePrintf(Mac_to_PHY_send_frame_buffer);
    return frame_len;
}
// 计算当前时刻所有活跃用户的 Jain's Index
// 修改函数签名，增加 int *active_users 参数
double Calculate_Jains_Fairness(int *active_users) {
    double sum_R = 0;
    double sum_R_sq = 0;
    int count = 0; // 改用局部变量 count

    for (int w = 0; w < WAVE_NUM; w++) {
        for (int u = 0; u < PER_WAVE_MAX_NUM; u++) {
            UserInfo_Block *ue = &wave_ueser_mapPtrArray[w][u];
            if (ue->State == 1) {
                double R = ue->Avg_Data_Rate;
                sum_R += R;
                sum_R_sq += (R * R);
                count++;
            }
        }
    }

    // 将计算出的数量通过指针传回
    if (active_users) *active_users = count;

    if (count == 0 || sum_R_sq == 0) return 0.0;
    
    return (sum_R * sum_R) / (count * sum_R_sq);
}