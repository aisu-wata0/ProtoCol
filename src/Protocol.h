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

typedef struct {
	uint size:5; // of data
	uint seq:6;
	msg_type_t type:5;
	uint8_t* data_p;
	uint8_t parity:8; // of data
	uint8_t error;
} packet;
// framefra(8) sizes(5) seq.seq(6) typet(5) data... parity(8)
// frame sizeseq seqtypet

void print(packet msg){
	printf("size=%x; seq=%x; type=%x\n", msg.size, msg.seq, msg.type);
	for(int i=0; i < msg.size; i++){
		printf("%hhx ", msg.data_p[i]);
	}
	printf("parity=%hhx ", msg.parity);
	printf("\n");
}

/**
 * @brief Serializes given message to buffer
 * @param msg Message in packet type
 * @param buf buffer to put msg data
 * @return size of buffer containing msg
 */
int serialize(packet msg, uint8_t** buf){
	int buf_n;
	buf_n = 3 + msg.size;
	
	buf = (uint8_t*)malloc(buf_n+1);
	
	buf[0] = framing_bits;

	/*size = 0x3;
	size == 0b00011;*/
	//buf[1] = (uint8_t)msg.size << 3;
	buf[1] = msg.size << 3;
	buf[1] = buf[1] & msg.seq >> 3;
	
	buf[2] = msg.seq << 5;
	buf[2] = buf[2] & msg.type;

	memcpy(&buf[3], msg.data_p, msg.size);
	
	if(msg.size > 0){
		buf_n += 1;
		buf[buf_n -1] = msg.parity;
	}
	
	return buf_n;
}

/**
 * @brief Uses bits in buf to make a packet
 */
packet deserialize(uint8_t* buf, int buf_n){
// sizesseq seqtypet datadata... paritypa
	packet msg;
	
	printf("Deserializing\n");
	for(int j = 0; j < 5; j++){
		printf("%hhx ", buf[j]);
	}
	printf("\n");
	
	msg.error = false;
	
	msg.size = buf[0] >> 3;
	msg.seq = (buf[0] & 0x3) | (buf[1] >> 5);
	msg.type = (buf[1] & 0b00011111);
	printf("size=%d; seq=%d; type=%d;\n", msg.size, msg.seq, msg.type);
	
	if(buf_n < 2+msg.size){
		fprintf(stderr, "received message buffer too small for said message size");
		msg.error = true;
	}
	
	msg.data_p = (uint8_t*)malloc(msg.size);
	// copy buffer to data
	memcpy(msg.data_p, &buf[2], msg.size);
	printf("[0]=%hhx [1]=%hhx [2]=%hhx\n", msg.data_p[0], msg.data_p[1], msg.data_p[2]);
	
	if(msg.size > 0){
		msg.parity = buf[2+msg.size];
	}
	
	return msg;
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