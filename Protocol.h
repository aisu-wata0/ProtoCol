#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if.h>

#define buf_max 65536
#define framing_bits 0b11000011
#define frame_n 8

typedef struct {
	int size:4;
	int seq:;
	unsigned char* data;
} packet;

#endif // PROTOCOL_H