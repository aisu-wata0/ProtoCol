#ifndef MASTER_H
#define MASTER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "Protocol.h"
#include "Socket.h"
#include "SlidingWindow.h"

#define COMMAND_BUF_SIZE 256

void parse(packet msg){
	
}

int dorei(char* device){
	Slider slider;
	slider_init(&slider, device);
	uint64_t rec_bytes;
	char command[COMMAND_BUF_SIZE];
	char* filename;
	packet response;
	packet msg;
	
	scanf("%[^\n]%*c", command);
	
	while(strcmp(command, "exit") != 0){
		msg.type = command_to_type(command, &filename);
		msg.size = strlen(filename);
		msg.data_p = malloc(msg.size);
		memcpy(msg.data_p, filename, msg.size);
		
		/*DEBUG*/
		response = sl_talk(&slider, msg);
		/**
		read_msg(&response);
		slider.rseq = seq_mod(slider.rseq +1);
		/**/
		
		printf("< response:\n");
		print(response);
		
		if(response.type == tam){
			FILE* stream = fopen("IO/out.txt","wb");
			
			int file_size_B = *(uint64_t*)response.data_p;
			
	//		if(ok)// TODO has space on current dir
				rec_bytes = receive_data(&slider, stream, file_size_B);
			printf("\nbytes transfered = %lu\n", rec_bytes);
			
			fclose(stream);
		} else {
			// TODO: if error, print on screen appropriately
			printf("command failed on server\n");
		}
		
		
		scanf("%[^\n]%*c", command);
	}
	
	return 0;
}

#endif