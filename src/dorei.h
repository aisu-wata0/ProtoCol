#ifndef DOREI_H
#define DOREI_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "Protocol.h"
#include "Socket.h"

void parse(packet msg){
	
}

int dorei(char* device){
	int sock = raw_socket_connection(device);
	uint8_t* buf = (uint8_t*)malloc(BUF_MAX); //to receive data
	
	packet msg;
	
	while(true){
		while(rec_packet(sock, &msg, buf) == FAIL);
		
		print(msg);
		
		parse(msg);
	}
	return 0;
}

#endif