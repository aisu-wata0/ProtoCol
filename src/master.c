#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Protocol.h"
#include "Socket.h"

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

void parse(packet msg){
	
}

	// sendto(sock, response, sizeof(response), 0, (struct sockaddr *)&sender, sendsize);

//int main(){
//	int sock_r = RawSocketConnection("eth0");
//	
//	unsigned char* buffer = (unsigned char *) malloc(buf_max); //to receive data
//	
//	packet msg;
//	
//	while(true){
//		while(rec_packet(&msg, buffer) == -1);
//		if(error(msg)){
//			send_nack(msg);
//		} else{
//			parse(msg);
//		}
//	}
//	return 0;
//}

