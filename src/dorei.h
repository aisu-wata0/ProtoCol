#ifndef MASTER_H
#define MASTER_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Protocol.h"
#include "Socket.h"


int dorei(){
	int sock = RawSocketConnection("eth0");
	uint8_t* buf = (uint8_t*)malloc(buf_max); //to send data
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