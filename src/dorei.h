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

bool process(Slider* slider, packet msg){
	chkMsgSeq(slider, msg);
	
	packet my_response = NIL_MSG;
	FILE* stream;
	char* command;
	msg_to_command(msg, &command);
	printf("%s\n", command);
	free(command);
	
	switch(msg.type){
		case cd:
			break;
			
		case ls:
			break;
			
		case get:
			stream = fopen((char*)msg.data_p,"rb");
			if(stream == NULL){
				fprintf(stderr, "FAIL: fopen() errno %d\n", errno);
				set_data(&my_response, acess);
			} else {
				struct stat sb;
				if (stat((char*)msg.data_p, &sb) == -1) {
					fprintf(stderr, "FAIL: stat() errno %d\n", errno);
					set_data(&my_response, acess);
					fclose(stream);
				} else {
					my_response.type = tam;
					set_data(&my_response, sb.st_size);
					
					printf("file size = %lu bytes\n", *(uint64_t*)my_response.data_p);
					packet msg = sl_talk(slider, my_response);
					if(msg.type == ok){
						send_data(slider, stream);
						fclose(stream);
						return true;
					}
					fclose(stream);
					return false;
				}
			}
			break;
			
		case put:
			break;
			
		default:
			return false;
	}
	my_response.type = error;
	sl_send(slider, &my_response);
	return false;
}

int dorei(char* device){
	Slider slider;
	slider_init(&slider, device);
	
	packet msg;
	
	while(true){
		sl_recv(&slider, &msg, 0);
		if(DEBUG_W)printf("Received request: ");
		if(DEBUG_W)print(msg);
		
		process(&slider, msg);
	}
	
	return 0;
}

#endif