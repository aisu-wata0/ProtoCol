#include <stdlib.h>
#include <stdio.h>

#include "master.h"
#include "dorei.h"

int main(int argc, char **argv) {
	int c;
	char* device;
	
	while (( c = getopt(argc, argv, "md")) != -1){
		switch (c){
			case 'n':
				device = optarg;
				break;
			case 'm':
				master(device);
				break;
			case 'd':
				dorei(device);
				break;
			default:
				fprintf(stderr, "Usage: %s [-m, run as master|-d, run as slave]\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	return 0;
}