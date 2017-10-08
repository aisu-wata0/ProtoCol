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
	
	FILE* stream = fopen("IO/out.txt","wb");
	
	int bytes;
	bytes = receive_data(&slider, stream, 9);
	printf("\nbytes transfered = %x\n", bytes);
	
	return 0;
}

#endif