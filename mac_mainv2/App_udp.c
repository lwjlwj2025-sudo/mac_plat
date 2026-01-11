#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if.h> //<net/if.h>
#include <assert.h>
#include "App_udp.h"
#include "App_queue.h"

int log_fd = 0;
struct sockaddr_in log_seraddr = {0};

void UDP_log_init()
{
    // 1. 创建通信的套接字
    log_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (log_fd == -1)
    {
        perror("socket");
        exit(0);
    }

    // 初始化服务器地址信息
    log_seraddr.sin_family = AF_INET;
    log_seraddr.sin_port = htons(IP_LOG_PORT);
    inet_pton(AF_INET, IP_LOG_ADDR, &log_seraddr.sin_addr.s_addr);
}

int UDP_Write(unsigned char *data, unsigned short len)
{
    sendto(log_fd, data, len, 0, (struct sockaddr *)&log_seraddr, sizeof(log_seraddr));
}

void WriteLog(const char *format, ...)
{
    char LogBuffer[1024] = {0};
    int FormatLen = 0;
    va_list aptr;
    va_start(aptr, format);
    FormatLen = vsprintf(LogBuffer, format, aptr);
    va_end(aptr);

    UDP_Write(LogBuffer, FormatLen);
    return;
}

int Send_rlc_fd = 0;
int Send_mac_u_fd = 0;
int Send_rrc_fd = 0;
struct sockaddr_in UDP_send_rlc_seraddr = {0};
struct sockaddr_in UDP_send_mac_u_seraddr = {0};
struct sockaddr_in UDP_send_rrc_seraddr = {0};

void UDP_write_init()
{
    // 创建发送数据通信的套接字
    Send_rlc_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (Send_rlc_fd == -1)
    {
        perror("socket");
        exit(0);
    }
    // 初始化服务器地址信息
    UDP_send_rlc_seraddr.sin_family = AF_INET;
    UDP_send_rlc_seraddr.sin_port = htons(IP_DEST_RLC_PORT);
    inet_pton(AF_INET, IP_DEST_RLC_ADDR, &UDP_send_rlc_seraddr.sin_addr.s_addr);

    // 创建发送控制指令通信的套接字
    Send_mac_u_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (Send_mac_u_fd == -1)
    {
        perror("socket");
        exit(0);
    }
    // 初始化服务器地址信息
    UDP_send_mac_u_seraddr.sin_family = AF_INET;
    UDP_send_mac_u_seraddr.sin_port = htons(IP_DEST_MAC_U_PORT);
    inet_pton(AF_INET, IP_DEST_MAC_U_ADDR, &UDP_send_mac_u_seraddr.sin_addr.s_addr);

    // 创建发送控制指令通信的套接字
    Send_rrc_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (Send_rrc_fd == -1)
    {
        perror("socket");
        exit(0);
    }
    // 初始化服务器地址信息
    UDP_send_rrc_seraddr.sin_family = AF_INET;
    UDP_send_rrc_seraddr.sin_port = htons(IP_DEST_RRC_PORT);
    inet_pton(AF_INET, IP_DEST_RRC_ADDR, &UDP_send_rrc_seraddr.sin_addr.s_addr);
}

void UDP_write_rlc(unsigned char *data, unsigned short len)
{
    sendto(Send_rlc_fd, data, len, 0, (struct sockaddr *)&UDP_send_rlc_seraddr, sizeof(UDP_send_rlc_seraddr));
}

void UDP_write_mac_u(unsigned char *data, unsigned short len)
{
    sendto(Send_mac_u_fd, data, len, 0, (struct sockaddr *)&UDP_send_mac_u_seraddr, sizeof(UDP_send_mac_u_seraddr));
}

void UDP_write_rrc(unsigned char *data, unsigned short len)
{
    sendto(Send_rrc_fd, data, len, 0, (struct sockaddr *)&UDP_send_rrc_seraddr, sizeof(UDP_send_rrc_seraddr));
}

int UDP_recv_rlc_fd = 0;
struct sockaddr_in UDP_recv_rlc_seraddr = {0};
struct sockaddr_in rlc_cliaddr;
int rlc_cliaddr_len = sizeof(rlc_cliaddr);

int UDP_recv_mac_u_fd = 0;
struct sockaddr_in UDP_recv_mac_u_seraddr = {0};
struct sockaddr_in mac_u_cliaddr;
int mac_u_cliaddr_len = sizeof(mac_u_cliaddr);

int UDP_recv_rrc_fd = 0;
struct sockaddr_in UDP_recv_rrc_seraddr = {0};
struct sockaddr_in rrc_cliaddr;
int rrc_cliaddr_len = sizeof(rrc_cliaddr);

void UDP_recv_init()
{
    // 创建服务器数据面通信的套接字
    UDP_recv_rlc_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (UDP_recv_rlc_fd == -1)
    {
        perror("socket");
        exit(0);
    }
    else                                                             //
    {                                                                //
        printf("socket success id for rlc: %d \n", UDP_recv_rlc_fd); //
    } //
    // 通信的套接字和本地的IP与端口绑定
    UDP_recv_rlc_seraddr.sin_family = AF_INET;
    UDP_recv_rlc_seraddr.sin_port = htons(IP_HOST_RLC_PORT); // 大端
    UDP_recv_rlc_seraddr.sin_addr.s_addr = INADDR_ANY;       // 0.0.0.0

    // 在sockfd_one绑定bind之前，设置其端口复用
    int data_opt = 1;
    setsockopt(UDP_recv_rlc_fd, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&data_opt, sizeof(data_opt));
    int data_ret = bind(UDP_recv_rlc_fd, (struct sockaddr *)&UDP_recv_rlc_seraddr, sizeof(UDP_recv_rlc_seraddr));
    if (data_ret == -1)
    {
        perror("bind");
        exit(0);
    }

    // 创建服务器控制面通信的套接字
    UDP_recv_mac_u_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (UDP_recv_mac_u_fd == -1)
    {
        perror("socket");
        exit(0);
    }
    else                                                                 //
    {                                                                    //
        printf("socket success id for mac_u: %d \n", UDP_recv_mac_u_fd); //
    } //
    // 通信的套接字和本地的IP与端口绑定
    UDP_recv_mac_u_seraddr.sin_family = AF_INET;
    UDP_recv_mac_u_seraddr.sin_port = htons(IP_HOST_MAC_U_PORT); // 大端
    UDP_recv_mac_u_seraddr.sin_addr.s_addr = INADDR_ANY;         // 0.0.0.0
    // 在sockfd_one绑定bind之前，设置其端口复用
    int control_opt = 1;
    setsockopt(UDP_recv_mac_u_fd, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&control_opt, sizeof(control_opt));
    int control_ret_mac = bind(UDP_recv_mac_u_fd, (struct sockaddr *)&UDP_recv_mac_u_seraddr, sizeof(UDP_recv_mac_u_seraddr));
    if (control_ret_mac == -1)
    {
        perror("bind");
        exit(0);
    }

    // 创建服务器控制面通信的套接字

    UDP_recv_rrc_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (UDP_recv_rrc_fd == -1)
    {
        perror("socket");
        exit(0);
    }
    else                                                             //
    {                                                                //
        printf("socket success id for rrc: %d \n", UDP_recv_rrc_fd); //
    } //
    // 通信的套接字和本地的IP与端口绑定
    UDP_recv_rrc_seraddr.sin_family = AF_INET;
    UDP_recv_rrc_seraddr.sin_port = htons(IP_HOST_RRC_PORT); // 大端
    UDP_recv_rrc_seraddr.sin_addr.s_addr = INADDR_ANY;       // 0.0.0.0
    // 在sockfd_one绑定bind之前，设置其端口复用
    int control_opt_rrc = 1;
    setsockopt(UDP_recv_rrc_fd, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&control_opt_rrc, sizeof(control_opt_rrc));

    int control_ret = bind(UDP_recv_rrc_fd, (struct sockaddr *)&UDP_recv_rrc_seraddr, sizeof(UDP_recv_rrc_seraddr));
    if (control_ret == -1)
    {
        perror("bind");
        exit(0);
    }
    // else
    // {
    //     printf("success %d\n", IP_DEST_RRC_PORT); //
    // }
}

unsigned int UDP_recv_rlc(unsigned char *data, unsigned short len)
{

    unsigned int rlen = recvfrom(UDP_recv_rlc_fd, data, len, 0, (struct sockaddr *)&rlc_cliaddr, &rlc_cliaddr_len);

    return rlen;
}

unsigned int UDP_recv_mac_u(unsigned char *data, unsigned short len)
{
    unsigned int rlen = recvfrom(UDP_recv_mac_u_fd, data, len, 0, (struct sockaddr *)&mac_u_cliaddr, &mac_u_cliaddr_len);
    return rlen;
}
unsigned int UDP_recv_rrc(unsigned char *data, unsigned short len)
{

    unsigned int rlen = recvfrom(UDP_recv_rrc_fd, data, len, 0, (struct sockaddr *)&rrc_cliaddr, &rrc_cliaddr_len);

    return rlen;
}
unsigned int close_rlc()
{

    if (!close(UDP_recv_rlc_fd))
        return 0;
}

void SetIP()
{
    int fd;
    struct ifreq ifr;
    struct sockaddr_in *addr;

    // 创建一个 socket 用于 ioctl
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
    {
        perror("socket");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 指定网口名称
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);

    // 获取当前接口的设置
    if (ioctl(fd, SIOCGIFADDR, &ifr) == -1)
    {
        perror("ioctl");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 设置新的 IP 地址
    addr = (struct sockaddr_in *)&ifr.ifr_addr;
    addr->sin_family = AF_INET;
    inet_pton(AF_INET, IP_HOST_ADDR, &addr->sin_addr);

    // 应用新的 IP 地址
    if (ioctl(fd, SIOCSIFADDR, &ifr) == -1)
    {
        perror("ioctl");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
}

void UDP_all_init()
{
    // SetIP();
    // UDP_log_init();
    UDP_write_init();
    UDP_recv_init();
}