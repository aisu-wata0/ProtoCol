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
	struct sockaddr_in addr_dest;
	uint8_t* buf = (uint8_t*)malloc(BUF_MAX); //to send data
	
	buf[0] = framing_bits;
	buf[1] = framing_bits;
	buf[2] = framing_bits;
	buf[3] = framing_bits;
	buf[4] = framing_bits;
	buf[5] = framing_bits;
	packet msg = *((packet*)&buf[1]);
//	msg.size = 0x3;
//	msg.seq = 0x1;
//	
//	// data
//	buf[2] = 0b10101010;
//	buf[3] = 0b10101011;
//	buf[4] = 0b10101111;
	
	printf("size: %d; seq: %d;\n", msg.size, msg.seq);
	printf("[0]=%hhx [1]=%hhx [2]=%hhx\n", buf[2], buf[3], buf[4]);
	
	while(true){
		printf("."); // Debug
		fflush(stdout);
		sendto(sock, buf, sizeof(uint8_t)*16, 0, (struct sockaddr*)&addr_dest, sizeof(addr_dest));
		
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