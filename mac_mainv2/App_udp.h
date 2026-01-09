#ifndef _APP_UDP_H_
#define _APP_UDP_H_

#include <string.h>
#include "App_queue.h"

#define IP_LOG_ADDR "192.188.1.54"
#define IP_LOG_PORT 9999
// DEST是卫星
// HOST是用户

#define IP_DEST_RLC_ADDR "127.0.0.1"
#define IP_DEST_RLC_PORT 3000

#define IP_DEST_MAC_U_ADDR "192.188.1.103"
#define IP_DEST_MAC_U_PORT 2222

#define IP_DEST_RRC_ADDR "192.188.1.101"
#define IP_DEST_RRC_PORT 2000

#define IP_HOST_ADDR "192.188.1.201"
#define IP_HOST_RLC_PORT 2000
#define IP_HOST_RRC_PORT 1111

#define IP_HOST_MAC_U_PORT 6666

void UDP_log_init();
void WriteLog(const char *format, ...);

void UDP_write_init();
void UDP_write_rlc(unsigned char *data, unsigned short len);
void UDP_write_mac_u(unsigned char *data, unsigned short len);
void UDP_write_rrc(unsigned char *data, unsigned short len);

void UDP_recv_init();
unsigned int UDP_recv_rlc(unsigned char *data, unsigned short len);
unsigned int UDP_recv_mac_u(unsigned char *data, unsigned short len);
unsigned int UDP_recv_rrc(unsigned char *data, unsigned short len);

void UDP_all_init();
unsigned int close_rlc();

#endif //
