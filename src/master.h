#ifndef MASTER_H
#define MASTER_H

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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "Protocol.h"
#include "Socket.h"

//packet createMessage(){
//	packet tegami;
//	
//	tegami.frame = framing_bits;
//	tegami.size = 8;
//	tegami.seq = 0;
//	tegami.type = ls;
//	tegami.data = (uint8_t*)malloc(sizeof(uint8_t)*1);
//	tegami.parity = 0b10101010;	
//	tegami.data[0] = 0b10101010;
//	
//	return tegami;
//}

int master(char* device){
	int sock = raw_socket_connection(device);
	
	uint8_t* buf;
	
	packet msg;
	msg.size = 3;
	msg.seq = 0;
	msg.type = ls;
	
	print(msg);
	
	buf = serialize(msg);
	
	while(true){
		printf(".");
		fflush(stdout);
		send(sock, buf, sizeof(uint8_t)*16, 0);
		
//		if(error(msg)){
//			send_nack(msg);
//		} else {
//			parse(msg);
//		}
		sleep(1);
	}
	return 0;
}

#endif