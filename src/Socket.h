#ifndef SOCKET_H
#define SOCKET_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Protocol.h"

int raw_socket_connection(char* device)
{
	int raw_socket;
	struct ifreq ir;
	struct sockaddr_ll address;
	struct packet_mreq mr;

	raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));  	/*cria socket*/
	if (raw_socket == -1) {
		printf("Erro no Socket\n");
		exit(-1);
	}

	memset(&ir, 0, sizeof(struct ifreq));  	/*dispositivo eth0*/
	memcpy(ir.ifr_name, device, strlen(device));
	if (ioctl(raw_socket, SIOCGIFINDEX, &ir) == -1) {
		printf("Erro no ioctl\n");
		exit(-1);
	}


	memset(&address, 0, sizeof(address)); 	/*IP do dispositivo*/
	address.sll_family = AF_PACKET;
	address.sll_protocol = htons(ETH_P_ALL);
	address.sll_ifindex = ir.ifr_ifindex;
	if (bind(raw_socket, (struct sockaddr *)&address, sizeof(address)) == -1) {
		printf("Erro no bind\n");
		exit(-1);
	}


	memset(&mr, 0, sizeof(mr));          /*Modo Promiscuo*/
	mr.mr_ifindex = ir.ifr_ifindex;
	mr.mr_type = PACKET_MR_PROMISC;
	if (setsockopt(raw_socket, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1)	{
		printf("Erro ao fazer setsockopt\n");
		exit(-1);
	}

	return raw_socket;
}

#endif // SOCKET_H