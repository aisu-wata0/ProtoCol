#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Protocol.h"
#include "Socket.h"

packet createMessage(){
	packet tegami;
	
	tegami.frame = framing_bits;
	tegami.size = 8;
	tegami.seq = 0;
	tegami.type = 3;
	tegami.data = (uint8_t*)malloc(sizeof(uint8_t)*1);
	tegami.parity = 0b10101010;	
	tegami.data[0] = 0b10101010;
	
	return tegami;
}

int main(){
	int sock_r = RawSocketConnection("eth0");
	const void* tegami;
	unsigned char* msg = (unsigned char *) malloc(buf_max); //to send data
	sendto(sock_r, response, sizeof(response), 0, (struct sockaddr *)&sender, sendsize);
	sendto(sock_r, tegami, tegami.size + 32);
	
	return 0;
}

