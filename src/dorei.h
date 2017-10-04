#ifndef DOREI_H
#define DOREI_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "Protocol.h"
#include "Socket.h"

/**
 * @brief Finds the start of the message
 * @return position [i] where the message starts, -1 if no message
 */
int frame_msg(uint8_t* buf, int buf_n){
	printf("Received message, searching frame\n");
	for(int i = 0; i < buf_n; i++){
		for (int bit = 0; bit <= frame_n-1; bit++){
			uint8_t pattern = buf[i] << bit;
			pattern |= buf[i+1] >> (frame_n -bit);
			if (pattern == framing_bits){
				shift_left(&buf[i], (buf_n-1) -(i) + 1, bit);
				printf("found at: %d\n", i);
				return i;
			}
		}
	}
	printf("none found\n\n");
	return FAIL;
}

/**
 * @brief Uses bits in buf to make a packet
 */
packet deserialize_msg(uint8_t* buf, int buf_n){
	packet msg;

	msg.size = buf[0] >> 4;
	msg.seq = (buf[0] << 4) >> 4;
	printf("size: %d; seq: %d;\n", msg.size, msg.seq);
	
	msg = *(packet*)&buf[0];
	printf("size: %d; seq: %d;\n", msg.size, msg.seq);
	
	msg.size = ((packet*)&buf[0])->size;
	msg.seq = ((packet*)&buf[0])->seq;
	printf("size: %d; seq: %d;\n", msg.size, msg.seq);
	
	msg.data_p = (uint8_t*)malloc(msg.size);
	
	// copy buffer to data
	memcpy(msg.data_p, &buf[1], msg.size);
	printf("data 0: %hhx; data 1: %hhx; data 2: %hhx", msg.data_p[0], msg.data_p[1], msg.data_p[2]);
	return msg;
}

/**
 * @brief Receives packet from raw socket
 * @param sock target socket
 * @param msg by reference
 * @param buf previously allocated buffer
 * @return 
 */
int rec_packet(int sock, packet* msg, uint8_t* buf){
	struct sockaddr saddr;
	int saddr_len = sizeof(saddr);
	
	memset(buf, 0, BUF_MAX);
	int buf_n = 0;
	while(buf_n == 0){
		buf_n = recvfrom(sock, buf, BUF_MAX, 0, &saddr, (socklen_t *)&saddr_len);
	} // receive a network packet and copy in to buffer

	int msg_start;
	msg_start = frame_msg(buf, buf_n);
	
	if(msg_start != FAIL){
		*msg = deserialize_msg(&buf[msg_start], (buf_n-1) -msg_start +1);
	}
	
	return msg_start;
}

void parse(packet msg){
	
}

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

int dorei(){
	int sock = raw_socket_connection("eth0");
	uint8_t* buf = (uint8_t*)malloc(BUF_MAX); //to receive data
	
	packet msg;
	
	while(true){
		while( rec_packet(sock, &msg, buf) == FAIL );
		
//		if(error(msg)){
//			send_nack(msg);
//		} else {
//			parse(msg);
//		}
		sleep(1); // Debug
	}
	return 0;
}

#endif