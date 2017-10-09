#ifndef MASTER_H
#define MASTER_H

#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <memory.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

#include "Protocol.h"
#include "Socket.h"
#include "SlidingWindow.h"

int master(char* device){
	Slider slider;
	slider_init(&slider, device);
	
	packet msg = sl_recv(&slider);
	
	print(msg);
	
	if(msg.type == get){
		printf("get \"%s\"", msg.data_p);
		
		FILE* stream = fopen((char*)msg.data_p,"rb");
		
		struct stat sb;
		if (stat((char*)msg.data_p, &sb) == -1) {
			fprintf(stderr, "file byte count with stat() error");
		}
		
		msg.type = tam;
		set_data(&msg, sb.st_size);
		
		printf("file size = %lu bytes\n", *(uint64_t*)msg.data_p);
		
		printf("> sending\n");
		print(msg);
		packet response = sl_send(&slider, msg);
		printf("< response\n");
		print(response);
		
		if(response.type == ok){
			send_data(&slider, stream);
		}
	}
	
	return 0;
}

#endif