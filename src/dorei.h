#ifndef DOREI_H
#define DOREI_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Protocol.h"
#include "Socket.h"
#include "SlidingWindow.h"

void parse(packet msg){
	
}

int dorei(char* device){
	Slider slider;
	slider_init(&slider, device);
	uint64_t rec_bytes;
	
	char* filename = "IO/out.txt";
	
	packet msg; msg.type = get;
	msg.size = strlen(filename);
	msg.data_p = malloc(msg.size);
	memcpy(msg.data_p, filename, msg.size);
	
	packet response;
	response = sl_send(&slider, msg);
	
	if(response.type == tam){
		FILE* stream = fopen("IO/out.txt","wb");
		
		int file_size_B = *(uint64_t*)response.data_p;
		
//		if(ok)// has space on current dir
			rec_bytes = receive_data(&slider, stream, file_size_B);
		
		fclose(stream);
	} else {
		// if error, print on screen appropriately
		// DEBUG
		fprintf(stderr, "response is not expected TAM");
	}
	
	printf("\nbytes transfered = %x\n", rec_bytes);
	
	return 0;
}

#endif