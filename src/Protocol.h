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
	uint8_t frame:8;
	uint8_t size:5;
	uint8_t seq:6;
	uint8_t type:5;
	uint8_t* data;
	uint8_t parity:8;
} packet;

#endif // PROTOCOL_H
