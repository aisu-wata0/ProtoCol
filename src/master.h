#ifndef MASTER_H
#define MASTER_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
			pattern += buf[i+1] >> frame_n -bit;
			if (pattern == framing_bits){
				shift(buf, i+1, bit);
				printf("found at: %d\n", i+1);
				return i+1;
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
int rec_packet(int sock, packet* msg, uint8_t* buf){
	struct sockaddr saddr;
	int saddr_len = sizeof(saddr);
	
	memset(buf, 0, BUF_MAX);
	int buf_n = 0;
	while(buf_n <= 0){
		buf_n = recvfrom(sock, buf, BUF_MAX, 0, &saddr, (socklen_t *)&saddr_len);
	} // receive a network packet and copy in to buffer

	int msg_start;
	msg_start = frame_msg(buf, buf_n);
	
	if(msg_start != FAIL){
		msg->size = buf[msg_start];
		msg->seq = buf[msg_start];
		//msg->size = &buf[msg_start](packet*)->size;
		//msg->seq = &buf[msg_start](packet*)->seq;
		printf("size: %d; seq: %d;\n", msg->size, msg->seq);
		
		//msg->data = (uint8_t*)malloc(msg->size); // TODO
		// copy buffer to data
	}
	
	return msg_start;
}

void parse(packet msg){
	
}

	// sendto(sock, response, sizeof(response), 0, (struct sockaddr *)&sender, sendsize);

int master(){
	int sock = RawSocketConnection("eth0");
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

//packet read_command(){
//	if(command_str = "ls"){
//		return;
//	}
//	if(command_str = "cd"){
//		return;
//	}
//}

#endif