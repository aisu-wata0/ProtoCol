#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <memory.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if.h>

#define FAIL -1

#define BUF_MAX 65536
#define framing_bits 0b01111110
#define frame_n 8

//typedef struct {
//	uint8_t frame:8;
//	uint size:5;
//	uint seq:6;
//	msg_type type:5;
//	uint8_t* data_p;
//	uint8_t parity:8;
//	uint8_t error;
//} packet;
//// framefra sizesseq seqtypet datadata... paritypa

typedef struct {
	uint8_t seq:4;
	uint8_t size:4;
	uint8_t* data_p;
} packet;

typedef enum msg_type {
	ack = 0x0,
	tam = 0x2,
	ok = 0x3,
	
	cd = 0x6,
	ls = 0x7,
	get = 0x8,
	put = 0x9,
	end = 0xa,
	
	screen = 0xc,
	data = 0xd,
	error = 0xe,
	nack = 0xf,
} msg_type_t;

enum error_code {
	err_inexistent = 0x1,
	err_access = 0x2,
	err_space = 0x3,
};

void print(packet msg){
	printf("size=%x; seq=%x; type=%x\n", msg.size, msg.seq);
	for(int i=0; i < msg.size; i++){
		printf("%hhx ", i, msg.data_p[i]);
	}
	printf("\n");
}

/**
 * @brief shifts buffer by b bits to the left
 */
void shift_left(uint8_t* buf, int buf_n, int bit){
	if(bit == 0) return;
	
	for(int i = 0; i < buf_n; i++){
		uint8_t byt = buf[i] << bit; // buf[i] if but == 0
		byt |= buf[i+1] >> (frame_n -bit); // 0 if bit == 0
		buf[i] = byt;
	}
/** shifting 1 bit left
 * 01234567 01234567 01234567
 * 12345670 01234567 01234567
 * 12345670 12345670 01234567...
 */
}

/**
 * @brief shifts buffer by b bits to the left
 */
void shift_right(uint8_t* buf, int buf_n, int bit){
	if(bit == 0) return;
	int i = 0;
	uint8_t byt_next = buf[i+1];
	
	for(i = 0; i < buf_n; i++){
		uint8_t byt = buf[i] << (frame_n -bit); // buf[i] if but == 0
		byt |= byt_next >> bit; // 0 if bit == 0
		// save bits that will be right shifted to use on next loop
		// before overwriting them below
		byt_next = buf[i+1];
		buf[i+1] = byt;
	}
/** shifting 1 bit right
 * 01234567 01234567
 * 01234567 01234567
 * 00123456 70123456 (7...
 */
}

#endif