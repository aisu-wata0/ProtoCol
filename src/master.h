#ifndef MASTER_H
#define MASTER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include "Protocol.h"
#include "Socket.h"
#include "SlidingWindow.h"
#include "commands.h"

bool parse(Slider* this, packet msg){
	packet response = NIL_MSG;
	uint64_t rec_bytes;
	bool succ = true;
	
	response = talk(this, msg, TIMEOUT);
	
	switch(msg.type){
		case cd:
			if(response.type != ok){
				printf("Command on other failed with errno %d", (*(int*)response.data_p));
			}
			
			break;
			
		case ls:
			if(response.type != ok){
				printf("Command on other failed with errno %d", (*(int*)response.data_p));
			}
			
			rec_bytes = receive_data(this, stdout);
			if(rec_bytes < 1){
				printf("Command failed on other with errno: %d\n", errno);
				succ = false;
				break;
			}
			printf("\nbytes transfered = %lu\n", rec_bytes);
			
			break;
			
		case get:
			if(response.type != tam){
				printf("Command failed on other with errno: %d\n", (*(int*)response.data_p));
				succ = false;
				break;
			}
			getC(this, (char*)msg.data_p, *(uint64_t*)response.data_p);
			
			break;
			
		case put:
			if(response.type != ok){
				printf("Command failed on other with errno: %d\n", (*(int*)response.data_p));
				succ = false;
				break;
			}
			putC(this, (char*)msg.data_p);
			
			break;
			
		default:
			succ = false;
			
			break;
	}
	
	unsetMsg(&response);
	return succ;
}

msg_type_t console (char** commands, int* lastCom, packet* msg) {
	char* filename;
	unsetMsg(msg);
	
	while (msg->type == invalid) {
		*lastCom = mod(*lastCom +1, COMMAND_HIST_SIZE);
		printf(" $\n"); // TODO print current remote dir
		int ret = scanf("%[^\n]%*c", commands[*lastCom]);
		if(ret < 0)
			fprintf(stderr, "Failed scanf(%s) with errno = %d", "%[^\n]%*c", errno);
		
		// if local command
		if(commands[*lastCom][0] == '!'){
			int ret = system(&commands[*lastCom][1]);
			if(ret < 0)
				fprintf(stderr, "Failed system(%s) with errno = %d", &commands[*lastCom][1], errno);
		} else { // if other command
			// filename is not a copy of command, it points to the same memory
			msg->type = command_to_type(commands[*lastCom], &filename);
		}
	}
	if (strlen(filename) > 0) {
		msg->size = strlen(filename)+1; // +1 null-terminator
		msg->data_p = malloc(msg->size);
		memcpy(msg->data_p, filename, msg->size);
	}
	return msg->type;
}

int master(char* device){
	Slider slider;
	slider_init(&slider, device);
	
	int lastCom = 0;
	char** commands;
	commands = malloc(COMMAND_HIST_SIZE*sizeof(char*));
	for(int i = 0; i < COMMAND_HIST_SIZE; ++i)
		commands[i] = malloc(COMMAND_BUF_SIZE*sizeof(char));
	
	packet msg = NIL_MSG;
	
	while(console(commands, &lastCom, &msg) != end){
		parse(&slider, msg);
	}
	
	return 0;
}

#endif