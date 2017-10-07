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

bool command_str(msg_type_t c, char* command){
	switch(c){
		case cd:
			strcpy(command, "cd ");
			break;
		case ls:
			strcpy(command, "ls ");
			break;
		default:
			command[0] = '\0';
			return false;
	}
	return true;
}

enum error_code {
	err_inexistent = 0x1,
	err_access = 0x2,
	err_space = 0x3,
};

#define framing_bits 0b01111110
#define frame_b 8
#define size_b 5
#define seq_b 6
#define type_b 5
#define parity_b 8

#define data_max (0x1 << size_b)-1 // (2^size_b)-1 maximum size of data
#define BUF_MAX 4 + data_max

typedef struct {
	uint size : size_b; // of data
	uint seq : seq_b;
	msg_type_t type : type_b;
	uint8_t* data_p;
	uint8_t parity : parity_b; // of data
	uint8_t error;
} packet;
// framefra(8) sizes(5) seq.seq(6) typet(5) data... parity(8)
// frame sizeseq seqtypet

uint8_t parity(packet msg){
	uint8_t par = 0x00;
	for(int i = 0; i < msg.size; i++){
		par ^= msg.data_p[i];
	}
	return par;
}

bool has_error(packet msg){
	return (parity(msg) == msg.parity);
}

/**
 * @brief Uses bits in buf to make a packet
 */
packet deserialize(uint8_t* buf, int buf_n){
// sizesseq seqtypet datadata... paritypa
	packet msg;
	
	msg.error = false;
	
	msg.size = buf[0] >> 3;
	msg.seq = (buf[0] & 0x3) | (buf[1] >> 5);
	msg.type = (buf[1] & 0b00011111);
	
	if(buf_n < 2+msg.size){
		// fprintf(stderr, "received message buffer too small for said message size");
		msg.error = true;
	}
	
	msg.data_p = (uint8_t*)malloc(msg.size);
	// copy buffer to data
	memcpy(msg.data_p, &buf[2], msg.size);
	
	if(msg.size > 0){
		msg.parity = buf[2+msg.size];
		msg.error = has_error(msg);
	}
	
	return msg;
}

/**
 * @brief Serializes given message to buffer
 * @param msg Message in packet type
 * @param buf buffer to put msg data
 * @return size of buffer containing msg
 */
int serialize(packet msg, uint8_t** buf_p){
// sizesseq seqtypet datadata... paritypa
	int buf_n;
	buf_n = 3 + msg.size;
	*buf_p = (uint8_t*)malloc(buf_n+1);
	
	uint8_t* buf = *buf_p;
	
	buf[0] = framing_bits;

	buf[1] = msg.size << 3;
	buf[1] = buf[1] | (msg.seq >> 3);
	
	buf[2] = msg.seq << 5;
	buf[2] = buf[2] | msg.type;

	memcpy(&(buf[3]), msg.data_p, msg.size);
	
	if(msg.size > 0){
		buf_n += 1;
		buf[buf_n-1] = parity(msg);
	}
	
	return buf_n;
}

void send_nack(int sock, packet msg){
	packet response;
	response.size = 0;
	response.type = nack;
}

void send_msg(int sock, packet msg){
	uint8_t* buf;
	int buf_n;
	
	buf_n = serialize(msg, &buf);

	send(sock, buf, buf_n, 0);
}

void print(packet msg){
	printf("size=%x; seq=%x; type=%x\n", msg.size, msg.seq, msg.type);
	for(int i=0; i < msg.size; i++){
		printf("%hhx ", msg.data_p[i]);
	}
	printf("parity=%hhx ", parity(msg));
	printf("\n");
}

/**
 * @brief shifts buffer by b bits to the left
 */
void shift_left(uint8_t* buf, int buf_n, int bit){
	if(bit == 0) return;
	
	for(int i = 0; i < buf_n; i++){
		uint8_t byt = buf[i] << bit; // buf[i] if but == 0
		byt |= buf[i+1] >> (frame_b -bit); // 0 if bit == 0
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
		uint8_t byt = buf[i] << (frame_b -bit); // buf[i] if but == 0
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

/**
 * @brief Finds the start of the message
 * @return position [i] where the message starts, -1 if no message
 */
int frame_msg(uint8_t* buf, int buf_n){
	for(int i = 0; i < buf_n; i++){
		for (int bit = 0; bit <= frame_b-1; bit++){
			uint8_t pattern = buf[i] << bit;
			pattern |= buf[i+1] >> (frame_b -bit);
			if (pattern == framing_bits){
				shift_left(&buf[i], (buf_n-1) -(i) + 1, bit);
				return i;
			}
		}
	}
	return FAIL;
}

/**
 * @brief Receives packet from raw socket
 * @param sock target socket
 * @param msg by reference
 * @param buf previously allocated buffer
 * @return 
 */
int rec_packet(int sock, packet* msg_p, uint8_t* buf){
	struct sockaddr saddr;
	int saddr_len = sizeof(saddr);

	int buf_n = 0;
	int msg_start = -1;
	
	while(msg_start != 0){
		memset(buf, 0, BUF_MAX);
		while(buf_n < 1){
			buf_n = recvfrom(sock, buf, BUF_MAX, 0, &saddr, (socklen_t *)&saddr_len);
		} // receive a network packet and copy in to buffer
		
		if(buf[0] == framing_bits){
			msg_start = 0;
		}
		//msg_start = frame_msg(buf, buf_n);
	}

	*msg_p = deserialize(&buf[msg_start+1], (buf_n-1) - (msg_start+1) +1);

	return msg_start;
}

/**
 * @brief Receives packet from raw socket
 * @param sock target socket
 * @param msg by reference
 * @param buf previously allocated buffer
 * @return 
 */
int try_packet(int sock, packet* msg_p, uint8_t* buf){
	struct sockaddr saddr;
	int saddr_len = sizeof(saddr);
	
	memset(buf, 0, BUF_MAX);
	int buf_n = 0;
	
	buf_n = recvfrom(sock, buf, BUF_MAX, MSG_DONTWAIT, &saddr, (socklen_t *)&saddr_len);
	// receive a network packet and copy in to buffer
	
	if(buf[0] != framing_bits){
		return 0;
	}

	*msg_p = deserialize(&buf[1], buf_n-1);

	return buf_n;
}

#endif