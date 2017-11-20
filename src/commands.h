#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <wordexp.h>
#include <time.h>
#include <errno.h>

#include "Protocol.h"
#include "Socket.h"
#include "SlidingWindow.h"

#define COMMAND_HIST_SIZE 64
#define COMMAND_BUF_SIZE 256

packet putC(Slider* slider, char* filename){
	packet reply = NIL_MSG;
	packet response = NIL_MSG;
	packet nextMsg = NIL_MSG;
	nextMsg.type = invalid;
	
	FILE* stream = NULL;
	
	stream = fopen(filename, "rb");
	if (stream == NULL) {
		fprintf(stderr, "INFO: fopen() errno %d\n", errno);
		set_data(&reply, acess);
		reply.type = error;
		nextMsg = say(slider, reply);
		
		return nextMsg;
	}
	struct stat sb;
	if (stat(filename, &sb) == -1) {
		fprintf(stderr, "INFO: stat() errno %d\n", errno);
		set_data(&reply, acess);
		reply.type = error;
		nextMsg = say(slider, reply);
		
		fclose(stream);
		return nextMsg;
	}
	// ready to send, inform file size
	reply.type = tam;
	set_data(&reply, sb.st_size);
	printf("file size = %lu bytes\n", *(uint64_t*)reply.data_p);
	
	response = talk(slider, reply, TIMEOUT);
	if(response.type != ok){
		
		fclose(stream);
		return nextMsg;
	}
	// other ready to receive
	send_data(slider, stream);
	
	fclose(stream);
	return nextMsg;
}

packet getC(Slider* slider, char* fout, unsigned long long fileSize) {
	packet reply = NIL_MSG;
	packet nextMsg = NIL_MSG;
	nextMsg.type = invalid;
	unsigned long long rec_bytes;
	
	printf("File size = %llu\n", fileSize);
	// Check if current directory has space
	struct statvfs currDir;
	statvfs(".", &currDir);
	unsigned long long freespace = currDir.f_bsize * currDir.f_bfree;
	if(freespace < fileSize){
		printf("Not enough space on current dir %llu/%llu\n", fileSize, freespace);
		reply.type = error;
		set_data(&reply, space);
		nextMsg = say(slider, reply);
		
		return nextMsg;
	}
	
	FILE* stream = fopen(fout, "wb");
	if (stream == NULL) {
		printf("Failed fopen(%s) with errno = %d\n", fout, errno);
		reply.type = error;
		set_data(&reply, acess);
		nextMsg = say(slider, reply);
		
		return nextMsg;
	}
	
	// everything ok, start sending
	rec_bytes = receive_data(slider, stream);
	if(rec_bytes < 1){
		printf("Receive data failed: wrong response. Try again\n");
		
		fclose(stream);
		return nextMsg;
	}
	printf("\nbytes transfered = %llu\n", rec_bytes);
	
	fclose(stream);
	return nextMsg;
}


#endif