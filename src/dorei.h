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
#include "commands.h"

void printDetails(FILE* stream, struct stat fileStat){
	struct passwd *pwd;
	struct group *grp;
	char detail[64];
	
	strcpy(detail, (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
	fprintf(stream, "%s", detail);
	strcpy(detail, (fileStat.st_mode & S_IRUSR) ? "r" : "-");
	fprintf(stream, "%s", detail);
	strcpy(detail, (fileStat.st_mode & S_IWUSR) ? "w" : "-");
	fprintf(stream, "%s", detail);
	strcpy(detail, (fileStat.st_mode & S_IXUSR) ? "x" : "-");
	fprintf(stream, "%s", detail);
	strcpy(detail, (fileStat.st_mode & S_IRGRP) ? "r" : "-");
	fprintf(stream, "%s", detail);
	strcpy(detail, (fileStat.st_mode & S_IWGRP) ? "w" : "-");
	fprintf(stream, "%s", detail);
	strcpy(detail, (fileStat.st_mode & S_IXGRP) ? "x" : "-");
	fprintf(stream, "%s", detail);
	strcpy(detail, (fileStat.st_mode & S_IROTH) ? "r" : "-");
	fprintf(stream, "%s", detail);
	strcpy(detail, (fileStat.st_mode & S_IWOTH) ? "w" : "-");
	fprintf(stream, "%s", detail);
	strcpy(detail, (fileStat.st_mode & S_IXOTH) ? "x" : "-");
	fprintf(stream, "%s", detail);
	fprintf(stream, " %lu ", fileStat.st_nlink);
	
	pwd = getpwuid(fileStat.st_uid);
	if(pwd == NULL) perror("getpwuid");
	else fprintf(stream, "%s ", pwd->pw_name);
	
	grp = getgrgid(fileStat.st_gid);
	if(grp == NULL) perror("getgrpid");
	fprintf(stream, "%s ", grp->gr_name);
	
	fprintf(stream, "%zu\t", fileStat.st_size);
	
	struct tm *time;
	char buffer[200];
	time = localtime(&fileStat.st_mtime);
	strftime(buffer, sizeof(buffer), "%b %e %H:%M", time);
	fprintf(stream, "%s ", buffer);
	
	return;
}

packet process(Slider* slider, packet msg){
	packet reply = NIL_MSG;
	packet nextMsg = NIL_MSG;
	packet response = NIL_MSG;
	nextMsg.type = invalid;
	
	FILE* stream = NULL;
	DIR *dir;

	char* commandStr = NULL;
	bool isCommand = false;
	isCommand = msg_to_command(msg, &commandStr);
	
	if( ! isCommand)
		return nextMsg;
	
	if(commandStr != NULL)
		printf("%s\n", commandStr);
	
	switch(msg.type){
		case cd:
			// https://linux.die.net/man/2/chdir
			if(chdir((char*)msg.data_p) < 0){
				switch(errno){
					case ENOENT:
						printf("Directory does not exist");
						set_data(&reply, inex);
						break;
					case EACCES:
						printf("Acces denied");
						set_data(&reply, acess);
						break;
					default:
						set_data(&reply, errno);
						printf("errno = %d", errno);
				}
				reply.type = error;
				nextMsg = say(slider, reply);
				break;
			}
			
			reply.type = ok;
			
			nextMsg = talk(slider, reply, 0);
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
				set_data(&reply, acess);
				reply.type = error;
				nextMsg = say(slider, reply);
				break;
			}
			char filename[COMMAND_BUF_SIZE] = ".tmp.ls.txt";
			stream = fopen(filename, "w");
			if (stream == NULL) {
				fprintf(stderr, "INFO: fopen(%s) errno %d\n", filename, errno);
				set_data(&reply, acess);
				reply.type = error;
				nextMsg = say(slider, reply);
				break;
			}
			// for all the files and directories within directory 
			struct dirent *ent;
			while ((ent = readdir(dir)) != NULL) {
				if(hidden == 0 && ent->d_name[0] == '.')
					continue;
				if(list){
					struct stat sb;
					stat(ent->d_name, &sb);
					printDetails(stream, sb);
				}
				fprintf(stream, "%s\n", ent->d_name);
			}
			closedir(dir);
			
			fclose(stream);
			stream = fopen(filename, "r");
			if (stream == NULL) {
				printf("Failed fopen(%s) with errno = %d\n", filename, errno);
				set_data(&reply, acess);
				reply.type = error;
				nextMsg = say(slider, reply);
				break;
			}
			
			reply.type = ok;
			response = talk(slider, reply, 0);
			if(response.type != ok){
				break;
			}
			
			long sentB = send_data(slider, stream); // TODO return
			printf("sent %ld bytes", sentB);
			
			break;
			
		case get:
			nextMsg = putC(slider, (char*)msg.data_p);
			break;
			
		case put:
			;
			reply.type = ok;
			// rec tam message
			response = talk(slider, reply, 0);
			nextMsg = getC(slider, (char*)msg.data_p, *(uint64_t*)response.data_p);
			break;
			
		default:
			break;
	}
	
	free(commandStr);
	return nextMsg;
}

int dorei(char* device){
	Slider slider;
	slider_init(&slider, device);
	
	packet msg = NIL_MSG;
	msg.type = invalid;
	
	while(true){
		/**
		int typ;
		scanf("%x", &typ);
		msg.type = typ;
		char target[256];
		scanf("%s", target);
		msg.size = strlen(target) + 1;
		msg.data_p = malloc(msg.size);
		memcpy(msg.data_p, target, msg.size);
		/**/
		if(msg.type == invalid){
			msg = receiveMsg(&slider);
		}
		/**/
		if(DEBUG_W)printf("Received request: ");
		if(DEBUG_W)print(msg);
		
		msg = process(&slider, msg);
	}
	
	return 0;
}

#endif