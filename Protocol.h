#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if.h>

#define buf_max 65536
#define framing_bits 0b01111110
#define frame_n 8

typedef struct {
	//int start:8; //framing_bits?
	int size:5;
	int seq:6;
	int type:5;
	unsigned char* data;
	//int parity:8;
} packet;

#endif // PROTOCOL_H
