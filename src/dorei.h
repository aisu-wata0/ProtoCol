#ifndef DOREI_H
#define DOREI_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Protocol.h"
#include "Socket.h"

//packet createMessage(){
//	packet tegami;
//	
//	tegami.frame = framing_bits;
//	tegami.size = 8;
//	tegami.seq = 0;
//	tegami.type = 3;
//	tegami.data = (uint8_t*)malloc(sizeof(uint8_t)*1);
//	tegami.parity = 0b10101010;	
//	tegami.data[0] = 0b10101010;
//	
//	return tegami;
//}

int dorei(){
	int sock = raw_socket_connection("eth0");
	uint8_t* buf = (uint8_t*)malloc(BUF_MAX); //to send data
	buf[0] = framing_bits;
	buf[1] = 0b00110001;
	
	while(true){
		printf("."); // Debug
		sendto(sock, buf, sizeof(uint8_t)*3, 0, (sockaddr*)&sender, sendsize);
		
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