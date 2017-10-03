#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if.h>

#define FAIL -1

#define BUF_MAX 65536
#define framing_bits 0b11000011
#define frame_n 8

//typedef struct {
//	uint8_t frame:8;
//	uint8_t size:5;
//	uint8_t seq:6;
//	uint8_t type:5;
//	uint8_t* data;
//	uint8_t parity:8;
//} packet;

typedef struct {
	uint8_t size:4;
	uint8_t seq:4;
	uint8_t* data;
} packet;

/**
 * @brief shifts buffer starting at position i, by b bits to the left
 */
void shift(uint8_t* buf, int i, int b){
	if(b == 0) return;
	
	for(; i < buf_n; i++){
		for (int bit = 0; bit <= frame_n-1; bit++){
			uint8_t byt = buf[i] << bit; // buf[i] if but == 0
			byt += buf[i+1] >> frame_n -bit; // 0 if bit == 0
			buf[i] = byt;
		}
	}
}

#endif