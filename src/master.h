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

#define COMMAND_HIST_SIZE 64
#define COMMAND_BUF_SIZE 256

bool parse(Slider* this, packet msg){
	packet response = NIL_MSG;
	char fout[COMMAND_BUF_SIZE];
	uint64_t rec_bytes;
	
	response = talk(this, msg, TIMEOUT);
	if(DEBUG_W)printf("< response:\n");
	if(DEBUG_W)print(response);
	
	switch(msg.type){
		case cd:
			break;
			
		case ls:
			break;
			
		case get:
			if(response.type != tam){
				printf("Command failed on server with errno: %d\n", (*(int*)response.data_p));
				return false;
			}
			unsigned long long file_size_B = *(uint64_t*)response.data_p;
// https://stackoverflow.com/questions/3992171/how-do-i-programmatically-get-the-free-disk-space-for-a-directory-in-linux
			// Check if current directory has space
			struct statvfs currDir;
			statvfs(".", &currDir);
			unsigned long long freespace = currDir.f_bsize * currDir.f_bfree;
			if(freespace < file_size_B){
				printf("Not enough space on current dir %llu/%llu\n", file_size_B, freespace);
				msg = NIL_MSG;
				msg.type = nack;
				msg = say(this, msg);
				return false;
			}
			memcpy(fout, msg.data_p, msg.size);
			strcat(fout, ".out");
			FILE* stream = fopen(fout, "wb");
			
			rec_bytes = receive_data(this, stream, file_size_B);
			if(rec_bytes < 1){
				printf("Receive data failed: wrong response. Try again\n");
				fclose(stream);
				return false;
			}
			printf("\nbytes transfered = %lu\n", rec_bytes);
			fclose(stream);
			
		case put:
			break;
			
		default:
			return false;
	}
	return true;
}

msg_type_t console (char** commands, int* lastCom, packet* msg) {
	char* filename;
	while (msg->type == invalid) {
		*lastCom = mod(*lastCom +1, COMMAND_HIST_SIZE);
		printf(" $\n"); // TODO print current remote dir
		scanf("%[^\n]%*c", commands[*lastCom]);
		printf("command: %s\n", commands[*lastCom]);
		// filename is not a copy of command, it points to the same memory
		msg->type = command_to_type(commands[*lastCom], &filename);
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
	
	packet msg;
	//int com_i = 0;
	
	// TODO: first thing when starting, send "cd ." command until you get a response
	// server will send on the data_p the dir after the cd command (current dir on server)
	// curr dir will be printed on the console (function below)
	
	/* TODO change scanf to getch
	 * read char by char and append them to command, if its \n, append "\0"
	 * goal: if up arrow pressed, printf("\33[2K\r"); erases current written line
	 * go back 1 in command history:
	 * com_i = mod(com_i -1, COMMAND_HIST_SIZE)
	 * printf("%s", command[com_i]);
	 * https://stackoverflow.com/questions/10463201/getch-and-arrow-codes */
	while(console(commands, &lastCom, &msg) != end){
		parse(&slider, msg);
	}
	
	return 0;
}

#endif