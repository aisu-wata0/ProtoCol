#ifndef DOREI_H
#define DOREI_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "Protocol.h"
#include "Socket.h"
#include "SlidingWindow.h"

void process(Slider* slider, packet msg){
	char* command;
	msg_to_command(msg, &command);
	printf("%s\n", command);
	free(command);
	
	if(msg.type == get){
		FILE* stream = fopen((char*)msg.data_p,"rb");
		if(stream == NULL){
			msg.type = error;
			msg.size = 0;
			// TODO set_data(&msg, error_code); 
			sl_send(slider, &msg);
		}
		
		struct stat sb;
		if (stat((char*)msg.data_p, &sb) == -1) {
			// fprintf(stderr, "file byte count with stat() error\n");
			msg.type = error;
			msg.size = 0;
			// TODO set_data(&msg, error_code); 
			sl_send(slider, &msg);
		} else {
			msg.type = tam;
			set_data(&msg, sb.st_size);
			
			printf("file size = %lu bytes\n", *(uint64_t*)msg.data_p);
			
			printf("> sending\n");
			print(msg);
			packet response = sl_talk(slider, msg);
			printf("< response\n");
			print(response);
			
			if(response.type == ok){
				send_data(slider, stream);
			}
		}
	}
}

int master(char* device){
	Slider slider;
	slider_init(&slider, device);
	
	packet msg;
	
	while(true){
		sl_recv(&slider, &msg, 0);
		if(DEBUG_W)printf("Received request: ");
		print(msg);
		
		process(&slider, msg);
	}
	
	return 0;
}

#endif