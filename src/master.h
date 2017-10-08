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

#include "Protocol.h"
#include "Socket.h"
#include "SlidingWindow.h"

int master(char* device){
	packet msg;
	
	Slider slider;
	slider_init(&slider, device);
	
	FILE* stream = fopen("IO/in.txt","rb");
	
	send_data(&slider, stream);
	
	return 0;
}

#endif