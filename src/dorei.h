#ifndef DOREI_H
#define DOREI_H

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

void printDetails(struct stat fileStat){
	struct passwd *pwd;
	struct group *grp;
	
	printf( (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
    printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
    printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
    printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
    printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
    printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");
	printf(" %lu ", fileStat.st_nlink);
	
	pwd = getpwuid(fileStat.st_uid);
	if(pwd == NULL) perror("getpwuid");
	else printf("%s ", pwd->pw_name);
	
	grp = getgrgid(fileStat.st_gid);
	if(grp == NULL) perror("getgrpid");
	printf("%s ", grp->gr_name);
	
	printf("%zu\t", fileStat.st_size);
	struct tm *time;
	char buffer[200];
	time = localtime(&fileStat.st_mtime);
	strftime(buffer, sizeof(buffer), "%b %e %H:%M", time);
	printf("%s ", buffer);
	return;
}

packet process(Slider* slider, packet msg){
	packet my_response = NIL_MSG;
	packet nextMsg = NIL_MSG;
	nextMsg.type = invalid;
	
	FILE* stream = NULL;
	DIR *dir;

	char* commandStr = NULL;
	bool isCommand = false;
	isCommand = msg_to_command(msg, &commandStr);	//msgSize = 0, problem
	
	if( ! isCommand) return nextMsg;
	
	if(commandStr != NULL) printf("%s\n", commandStr);
	
	switch(msg.type){
		case cd:
			// https://linux.die.net/man/2/chdir
			chdir((char*)msg.data_p);
			
			if(msg.error){
				//enviar Nack aonde?
				//enviar msg aonde?
				
				my_response.type = nack;
				break;
			}
			
			if(errno != 0){
				switch(errno){
					case ENOENT:
						printf("Directory does not exist");
						set_data(&my_response, inex);
						break;
					case EACCES:
						printf("Acces denied");
						set_data(&my_response, acess);
						break;
					default:
						printf("Errno = %d", errno);
				}
				my_response.type = error;
			}
			
			my_response.type = ok;
			
			break;
			
		case ls:
			// https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
			;
			int hidden = 0;
			int list = 0;
			
			wordexp_t argInfo;
			int opt;
			
			wordexp(commandStr, &argInfo, 0);
			
			optind = 0;
			while ((opt = getopt(argInfo.we_wordc, argInfo.we_wordv, "la")) != -1) {
				switch (opt) {
					case 'l':
						list = 1;
						break;
					case 'a':
						hidden = 1;
						break;
					default: /* '?' */
						fprintf(stderr, "WARN : only accepts -l | -a");
				}
			}
			wordfree(&argInfo);
	
			dir = opendir(".");
			if (dir == NULL) {
				 // could not open directory
				fprintf(stderr, "INFO: opendir() errno %d\n", errno);
				set_data(&my_response, acess);
				my_response.type = error;
				//nextMsg = talk(slider, my_response, 0);
				break;
			}
			// for all the files and directories within directory 
			struct dirent *ent;
			while ((ent = readdir(dir)) != NULL) {
				if(hidden == 0 && ent->d_name[0] == '.')
					continue;
				if(list){
					//sprintf(buf, "%s", ent->d_name);
					struct stat sb;
					stat(ent->d_name, &sb);
					printDetails(sb);
				}
				printf("%s\n", ent->d_name);
			}
			closedir(dir);
			break;
			
		case get:
			stream = fopen((char*)msg.data_p,"rb");
			if (stream == NULL) {
				fprintf(stderr, "INFO: fopen() errno %d\n", errno);
				set_data(&my_response, acess);
				my_response.type = error;
				nextMsg = talk(slider, my_response, 0);
				break;
			}
			struct stat sb;
			if (stat((char*)msg.data_p, &sb) == -1) {
				fprintf(stderr, "INFO: stat() errno %d\n", errno);
				set_data(&my_response, acess);
				my_response.type = error;
				nextMsg = talk(slider, my_response, 0);
				fclose(stream);
				break;
			}
			my_response.type = tam;
			set_data(&my_response, sb.st_size);
			
			printf("file size = %lu bytes\n", *(uint64_t*)my_response.data_p);
			packet msg = talk(slider, my_response, TIMEOUT);
			if(msg.type != ok){
				fclose(stream);
				break;
			}
			send_data(slider, stream);
			fclose(stream);
			break;
			
		case put:
			break;
			
		default:
			break;
	}
	free(commandStr);
	return nextMsg;
}

int dorei(char* device){
	Slider slider;
	//slider_init(&slider, device);
	
	packet msg = NIL_MSG;
	msg.type = invalid;
	
	while(true){
		int test;
		char target[256];
		scanf("%x", &test);
		scanf("%s", target);
		msg.type = test;
		msg.size = strlen(target) + 1;
		msg.data_p = malloc(msg.size * sizeof(char));
		strcpy((char*)msg.data_p, target);
		
		if(msg.type == invalid)
			recvMsg(&slider, &msg, 0);
		if(DEBUG_W)printf("Received request: ");
		if(DEBUG_W)print(msg);
		
		msg = process(&slider, msg);
	}
	
	return 0;
}

#endif