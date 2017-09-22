#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Protocol.h"
#include "Socket.h"


int main(){
	int sock_r = RawSocketConnection("eth0");
	unsigned char* msg = (unsigned char *) malloc(buf_max); //to send data
	// sendto(sock, response, sizeof(response), 0, (struct sockaddr *)&sender, sendsize);
	//sendto(int sockfd, const void *msg, int len, unsigned int flags,
	//const struct sockaddr *to, socklen_t tolen);
	return 0;
}

