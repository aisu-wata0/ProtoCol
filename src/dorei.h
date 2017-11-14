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

packet process(Slider* slider, packet msg){
	packet my_response = NIL_MSG;
	packet nextMsg = NIL_MSG;
	nextMsg.type = invalid;
	
	FILE* stream = NULL;
	char* command;
	msg_to_command(msg, &command);
	printf("%s\n", command);
	free(command);
	
	switch(msg.type){
		case cd:
			// https://linux.die.net/man/2/chdir
			break;
			
		case ls:
			// https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
			break;
			
		case get:
			stream = fopen((char*)msg.data_p,"rb");
			if (stream == NULL) {
				fprintf(stderr, "INFO: fopen() errno %d\n", errno);
				set_data(&my_response, acess);
				my_response.type = error;
				nextMsg = talk(slider, my_response, 0);
				break;
			}
			struct stat sb;
			if (stat((char*)msg.data_p, &sb) == -1) {
				fprintf(stderr, "INFO: stat() errno %d\n", errno);
				set_data(&my_response, acess);
				my_response.type = error;
				nextMsg = talk(slider, my_response, 0);
				fclose(stream);
				break;
			}
			my_response.type = tam;
			set_data(&my_response, sb.st_size);
			
			printf("file size = %lu bytes\n", *(uint64_t*)my_response.data_p);
			packet msg = talk(slider, my_response, TIMEOUT);
			if(msg.type != ok){
				fclose(stream);
				break;
			}
			send_data(slider, stream);
			fclose(stream);
			break;
			
		case put:
			break;
			
		default:
			break;
	}
	return nextMsg;
}

int dorei(char* device){
	Slider slider;
	slider_init(&slider, device);
	
	packet msg = NIL_MSG;
	msg.type = invalid;
	
	while(true){
		if(msg.type == invalid)
			recvMsg(&slider, &msg, 0);
		if(DEBUG_W)printf("Received request: ");
		if(DEBUG_W)print(msg);
		
		msg = process(&slider, msg);
	}
	
	return 0;
}

#endif