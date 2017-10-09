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

#include "Protocol.h"
#include "Socket.h"
#include "SlidingWindow.h"

int master(char* device){
	Slider slider;
	slider_init(&slider, device);
	
	char* filename = "../bfs15-lgjr15.tar.gz";
	
	FILE* stream = fopen(filename,"rb");
	
	struct stat sb;
	if (stat(filename, &sb) == -1) {
		fprintf(stderr, "file byte count with stat() error");
	}
	
	packet msg;
	set_data(&msg, sb.st_size);
	
	printf("file size = %llu bytes\n", *(uint64_t*)msg.data_p);
	
	// sl_send_msg(slider, msg);

	send_data(&slider, stream);
	
	return 0;
}

#endif