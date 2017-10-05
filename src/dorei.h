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
	printf("<<< Received message of size = %d, searching frame\n", buf_n);
	for(int i = 0; i < buf_n; i++){
		for (int bit = 0; bit <= frame_n-1; bit++){
			uint8_t pattern = buf[i] << bit;
			pattern |= buf[i+1] >> (frame_n -bit);
			if (pattern == framing_bits){
				printf("shifting by %d bits\n", bit);
				shift_left(&buf[i], (buf_n-1) -(i) + 1, bit);
				printf(">>> found at: %d\n", i);
				return i;
			}
		}
	}
	printf("none found\n\n");
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
	printf("+++++++++\n");
	struct sockaddr saddr;
	int saddr_len = sizeof(saddr);
	
	memset(buf, 0, BUF_MAX);
	int buf_n = 0;
	while(buf_n == 0){
		buf_n = recvfrom(sock, buf, BUF_MAX, 0, &saddr, (socklen_t *)&saddr_len);
	} // receive a network packet and copy in to buffer

	int msg_start;
	msg_start = frame_msg(buf, buf_n);
	
	if(msg_start != 0){
		return FAIL;
	}

	*msg_p = deserialize(&buf[msg_start], (buf_n-1) -msg_start +1);
	
	printf("------------ end rec pack\n");
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

int dorei(char* device){
	int sock = raw_socket_connection(device);
	uint8_t* buf = (uint8_t*)malloc(BUF_MAX); //to receive data
	
	packet msg;
	
	while(true){
		while(rec_packet(sock, &msg, buf) == FAIL);
		
		print(msg);
		
//		if(msg.error){
//			send_nack(msg);
//		} else {
//			parse(msg);
//		}
		sleep(1);
	}
	return 0;
}

#endif