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

int master(char* device){
	int sock = raw_socket_connection(device);
	
	char* data = "kaka!";
	
	packet msg;
	msg.size = strlen(data);
	msg.seq = 0;
	msg.type = ls;
	msg.data_p = (uint8_t*)data;
	
	print(msg);
	
	while(true){
		printf(".");
		fflush(stdout);
		send_msg(sock, msg);
		
		sleep(1);
	}
	return 0;
}

#endif