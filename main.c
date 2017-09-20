#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Socket.h"

#define buf_max 65536
#define framing_bits 0b11000011
#define frame_n 8

typedef struct {
	int size:4;
	int seq:;
	unsigned char* data;
} packet;

int frame_msg(unsigned char* buffer, int buf_n){
	for(int b = 0; b < buf_n-1; b++){
		for (int i = 0; i < frame_n; i++){
			char pattern = buffer[b] << i;
			pattern += buffer[b+1] >> frame_n -i;
			if (pattern == framing_bits)
				return b*frame_n + i;
		}
	}
	return -1;
}

int rec_packet(packet* msg, unsigned char* buffer){
	struct sockaddr saddr;
	int saddr_len = sizeof(saddr);
	memset(buffer, 0, buf_max);
	
	int buf_n = 0;
	while(buf_n <= 0){
		// printf(".");
		buf_n = recvfrom(sock_r, buffer, buf_max, 0, &saddr, (socklen_t *)&saddr_len);
	}// receive a network packet and copy in to buffer

	int msg_start;
	msg_start = frame_msg(buffer, buf_n);
	if(msg_start == -1){
		return -1;
	}
	
	//msg->size = buff[msg_start/frame_n] // TODO 
	return 0;
}

int main(){
	int sock_r = RawSocketConnection(char *device); // TODO: what device?
	
	unsigned char* buffer = (unsigned char *) malloc(buf_max); //to receive data
	
	packet msg;
	
	while(true){
		while(rec_packet(&msg, buffer) == -1);
		
		// do something with msg
	}
	
	// sendto(sock, response, sizeof(response), 0, (struct sockaddr *)&sender, sendsize);
	return 0;
}

