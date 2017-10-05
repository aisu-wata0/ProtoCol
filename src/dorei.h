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
 * @brief Uses bits in buf to make a packet
 */
packet deserialize_msg(uint8_t* buf, int buf_n){
	packet msg;
	printf("Deserializing\n");
	for(int j = 0; j < 5; j++){
		printf("%hhx ", buf[j]);
	}
	printf("\n");
	
	msg.size = buf[0] >> 4;
	msg.seq = buf[0] & 0b00001111;
	
	//printf("size: %d; seq: %d;\n", msg.size, msg.seq);

	msg = *(packet*)&buf[0];
	//printf("size: %d; seq: %d;\n", msg.size, msg.seq);
	
	msg.data_p = (uint8_t*)malloc(msg.size);
	
	// copy buffer to data
	memcpy(msg.data_p, &buf[1], msg.size);
	//printf("[0]=%hhx [1]=%hhx [2]=%hhx\n", msg.data_p[0], msg.data_p[1], msg.data_p[2]);
	return msg;
}

///**
// * @brief Uses bits in buf to make a packet
// */
//msg deserialize_msg(uint8_t* buf, int buf_n){
////// sizesseq seqtypet datadata... paritypa
//	packet msg;
//	
//	printf("Deserializing\n");
//	for(int j = 0; j < 5; j++){
//		printf("%hhx ", buf[j]);
//	}
//	printf("\n");
//	
//	msg.error = false;
//	
//	msg.size = buf[0] >> 3;
//	msg.seq = (buf[0] & 0x3) | (buf[1] >> 5);
//	msg.type = (buf[1] & 0b00011111);
//	printf("size=%d; seq=%d; type=%d;\n", msg.size, msg.seq, msg.type);
//	
//	if(buf_n < 2+msg.size){
//		fprintf(stderr, "received message buffer too small for said message size");
//		msg.error = true;
//	}
//	
//	msg.data_p = (uint8_t*)malloc(msg.size);
//	// copy buffer to data
//	memcpy(msg.data_p, &buf[2], msg.size);
//	printf("[0]=%hhx [1]=%hhx [2]=%hhx\n", msg.data_p[0], msg.data_p[1], msg.data_p[2]);
//	
//	if(msg.size > 0){
//		msg.parity = buf[2+msg.size];
//	}
//	
//	return true;
//}

void check_error(packet* msg){
}
//void check_error(packet* msg){
//	uint8_t par = 0x0;
//	for(int i = 0; i<msg.size; i++){
//		par ^= msg.data_p[i]
//	}
//	msg.error = (par == msg.parity);
//}

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
	
	if(msg_start != FAIL){
		*msg_p = deserialize_msg(&buf[msg_start], (buf_n-1) -msg_start +1);
		
		if(msg_p->size > 0){
			check_error(msg_p);
		}
	}
//	while(msg_start != FAIL){
//		*msg_p = deserialize_msg(&buf[msg_start], (buf_n-1) -msg_start +1);
//		printf("other frames\n");
//		int oldms = msg_start+1;
//		msg_start = frame_msg(&buf[msg_start+1], (buf_n-1) -(msg_start+1) + 1);
//		if(msg_start != FAIL){
//			msg_start += oldms;
//		}
//	}

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
		rec_packet(sock, &msg, buf);
		
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