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
	packet msg;
	
	Slider slider;
	slider_init(&slider, device);
	
	FILE* stream = fopen("out.txt","wb");
	
	printf("\nbytes transfered = %x\n", receive_data(&slider, stream, 9));
	
	return 0;
}

#endif