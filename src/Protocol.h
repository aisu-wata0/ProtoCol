#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <memory.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>

#define DEBUG_P false

#define TIMEOUT 2

#define FAIL -1
#define mod(X,Y) (((X) % (Y)) < 0 ? ((X) % (Y)) + (Y) : ((X) % (Y)))

typedef enum msg_type {
	ack = 0x0,
	tam = 0x2,
	ok = 0x3,
	
	cd = 0x6,
	ls = 0x7,
	get = 0x8,
	put = 0x9,
	end = 0xa,
	invalid = 0xb,
	screen = 0xc,
	data = 0xd,
	error = 0xe,
	nack = 0xf,
} msg_type_t;

typedef enum err_code {
	inex = 0x0,
	acess = 0x1,
	space = 0x1,
} err_code_t;

/**
 * @brief trims spaces from start and end of string, doesn't copy
 */
char* trim(char *str){
	if(str == NULL) { return NULL; }
	if(str[0] == '\0') { return str; }


	/* Move the front and back pointers to address the first non-whitespace
	 * characters from each end. */
	char* frontp = str;
	while( isspace((unsigned char)*frontp) ){
		frontp++;
	}
	
	size_t len = strlen(str);
	char* endp = str + len -1;
	if( endp != frontp ){
		while( isspace((unsigned char) *endp) && endp != frontp ) {
			endp--;
		}
	}

	if( str + len - 1 != endp ){
		*(endp + 1) = '\0'; // trim end
	} else if( frontp != str &&  endp == frontp ) {
		*str = '\0'; // empty str
	}

	/* Shift the string so that it starts at str so that if it's dynamically
	 * allocated, we can still free it on the returned pointer.  Note the reuse
	 * of endp to mean the front of the string buffer now. */
	endp = str;
	if(frontp != str){
		while(*frontp){
			*endp++ = *frontp++;
		}
		*endp = '\0';
	}

	return str;
}

/**
 * @brief returns true if x has y at the start of the string
 */
bool contains_at_start(char* x, char* y){
	for(int i=0; i < strlen(y); i++){
		if(x[i] != y[i]){
			return false;
		}
	}
	return true;
}

/**
 * @brief if input string is a valid command
 * target points to the start of the command parameters
 */
bool is_command(char* command, char* typ, char** target){
	if(contains_at_start(command, typ)){
		*target = &command[strlen(typ)];
		return true;
	}
	return false;
}
/**
 * @brief Parses a string into a msg_type of a valid command with target
 * pointing to after the arguments of the command
 * @param command string
 * @param target is not a copy of command, it points to the same memory
 * @return 
 */
msg_type_t command_to_type(char* command, char** target){
	trim(command);
	
	if(is_command(command, "cd", target)) return cd;
	if(is_command(command, "ls", target)) return ls;
	if(is_command(command, "get ", target)) return get;
	if(is_command(command, "put ", target)) return put;
	if(is_command(command, "exit", target)) return end;

	return invalid;
}

/**
 * @brief copies s1 and s2 to result, allocates necessary memory
 */
char* concat_to(const char* s1, const char* s2){
	char* result;
	const size_t len1 = strlen(s1);
	const size_t len2 = strlen(s2);
	
	result = malloc(len1+len2+1); // +1 null-terminator
	if(result == NULL) { fprintf(stderr, "ERROR: malloc returned NULL\n"); }
	
	memcpy(result, s1, len1);
	memcpy(result+len1, s2, len2+1); // +1 to copy the null-terminator
	
	return result;
}

enum error_code {
	err_inexistent = 0x1,
	err_access = 0x2,
	err_space = 0x3,
};

#define framing_bits 0b01111110
#define frame_b 8
#define size_b 5
#define seq_b 6
#define type_b 5
#define parity_b 8

#define data_max ((0x1 << size_b)-1) // (2^size_b)-1 maximum size of data
#define max_file_size ((0x1 << data_max)-1)

#define seq_max ((0x1 << seq_b)-1) // (2^size_b)-1 maximum size of seq

#define seq_mod(X) mod((X),(seq_max+1))

#define BUF_MAX (4 + data_max)

typedef struct {
	uint size : size_b; // of data
	uint seq : seq_b;
	msg_type_t type : type_b;
	uint8_t* data_p;
	uint8_t parity : parity_b; // of data
	uint8_t error;
} packet;
// framefra(8) sizes(5) seq.seq(6) typet(5) data... parity(8)
// frame sizeseq seqtypet
const packet NIL_MSG = { 0, 0, 0xb, NULL, 0, 0 };
/**
 * @brief calculates parity of message from its data
 * @param msg
 * @return 
 */
uint8_t parity(packet msg){
	uint8_t par = 0x00;
	for(int i = 0; i < msg.size; i++){
		par ^= msg.data_p[i];
	}
	return par;
}

bool has_error(packet msg){
	return (parity(msg) != msg.parity);
}

/**
 * @brief Uses bits in buf to make a packet
 */
packet deserialize(uint8_t* buf, int buf_n){
// sizesseq seqtypet datadata... paritypa
	packet msg;
	
	msg.error = false;
	
	msg.size = buf[0] >> 3;
	msg.seq = ((buf[0] & 0b111) << 3) | (buf[1] >> 5);
	msg.type = (buf[1] & 0b11111);
	
	if(buf_n < 2+msg.size){
		fprintf(stderr, "received message buffer too small for said message size\n");
		msg.error = true;
	}

	msg.data_p = (uint8_t*)malloc(msg.size);
	// copy buffer to data
	memcpy(msg.data_p, &buf[2], msg.size);
	
	if(msg.size > 0){
		msg.parity = buf[2+msg.size];
		if(DEBUG_P)printf("msg.parity %x calc parity %x\n", msg.parity, parity(msg));
		msg.error = has_error(msg);
		if(msg.error){
			if(DEBUG_P)printf("deserialized message has detected error %x\n", has_error(msg));
		}
	}
	
	return msg;
}

/**
 * @brief Serializes given message to buffer
 * @param msg Message in packet type
 * @param buf buffer to put msg data
 * @return size of buffer containing msg
 */
int serialize(packet msg, uint8_t* buf){
// sizesseq seqtypet datadata... paritypa
	int buf_n = 3 + msg.size;
	
	buf[0] = framing_bits;
	// 00012345
	buf[1] = msg.size << 3;
	// 12345000 | 123456 >> 3
	// 12345000 | 123
	// 12345123
	buf[1] = buf[1] | (msg.seq >> 3);
	// 00123456 << 5
	buf[2] = msg.seq << 5;
	// 45600000 | 00012345
	buf[2] = buf[2] | msg.type;
	// 45612345

	memcpy(&(buf[3]), msg.data_p, msg.size);
	
	if(msg.size > 0){
		buf_n += 1;
		buf[buf_n-1] = parity(msg);
	}
	// TMP
	if(true)printf("=== serializing %hhx %hhx %hhx \t", buf[0], buf[1], buf[2]);
	if(true)printf("msg.seq %x == %x buf_seq\n", msg.seq, ((buf[1] & 0b111) << 3) | (buf[2] >> 5));
	if(msg.seq != ( ((buf[1] & 0b111) << 3) | (buf[2] >> 5) ) ){
		if(true)printf("((buf[1] & 0b111) << 3) = %x\n(buf[2] >> 5) = %x\n", ((buf[1] & 0b111) << 3), (buf[2] >> 5));
	}
	return buf_n;
}

void print(packet msg){
	printf("[%x] Type = %x; Size = %x;\n", msg.seq, msg.type, msg.size);
	for(int i=0; i < msg.size; i++){
		printf("%hhx ", msg.data_p[i]);
	} printf(" Parity=%hhx; \tError=%x\n", parity(msg), msg.error);
	printf("%s ", (char*)msg.data_p);
	printf("\n");
}

/**
 * @brief shifts buffer by b bits to the left
 */
void shift_left(uint8_t* buf, int buf_n, int bit){
	if(bit == 0) return;
	
	for(int i = 0; i < buf_n; i++){
		uint8_t byt = buf[i] << bit; // buf[i] if but == 0
		byt |= buf[i+1] >> (frame_b -bit); // 0 if bit == 0
		buf[i] = byt;
	}
/** shifting 1 bit left
 * 01234567 01234567 01234567
 * 12345670 01234567 01234567
 * 12345670 12345670 01234567...
 */
}

/**
 * @brief shifts buffer by b bits to the left
 */
void shift_right(uint8_t* buf, int buf_n, int bit){
	if(bit == 0) return;
	int i = 0;
	uint8_t byt_next = buf[i+1];
	
	for(i = 0; i < buf_n; i++){
		uint8_t byt = buf[i] << (frame_b -bit); // buf[i] if but == 0
		byt |= byt_next >> bit; // 0 if bit == 0
		// save bits that will be right shifted to use on next loop
		// before overwriting them below
		byt_next = buf[i+1];
		buf[i+1] = byt;
	}
/** shifting 1 bit right
 * 01234567 01234567
 * 01234567 01234567
 * 00123456 70123456 (7...
 */
}

/**
 * @brief Finds the start of the message
 * @return position [i] where the message starts, -1 if no message
 */
int frame_msg(uint8_t* buf, int buf_n){
	for(int i = 0; i < buf_n; i++){
		for (int bit = 0; bit <= frame_b-1; bit++){
			uint8_t pattern = buf[i] << bit;
			pattern |= buf[i+1] >> (frame_b -bit);
			if (pattern == framing_bits){
				shift_left(&buf[i], (buf_n-1) -(i) + 1, bit);
				return i;
			}
		}
	}
	return FAIL;
}

void set_data(packet* msg, uint64_t num){
	msg->size = sizeof(uint64_t);
	msg->data_p = malloc(msg->size);
	*(uint64_t*)msg->data_p = num;
}
/**
 * @brief Receives a valid packet from raw socket
 * @param sock target socket
 * @param msg by reference
 * @param buf previously allocated buffer
 * @param timeout_sec timeout in secs, if 0, blocks until a packet is received
 * @return returns -1 if timed out, else, bytes written in buf
 */
int rec_packet(int sock, packet* msg_p, uint8_t* buf, int timeout_sec){
	struct sockaddr saddr;
	int saddr_len = sizeof(saddr);

	int msg_start;
	int buf_n = 0;
	int ended = false;
	
	while(!ended){
		memset(buf, 0, BUF_MAX);
		
		struct timeval tv;
		if(timeout_sec > 0){
			tv.tv_sec = timeout_sec; tv.tv_usec = 0;
			while(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval)) < 0){
				fprintf(stderr, "setsockopt failed\n");
				sleep(1);
			}
		}
		
		if(DEBUG_P)printf("recv...");
		buf_n = recvfrom(sock, buf, BUF_MAX, 0, &saddr, (socklen_t *)&saddr_len);
		// receive a network packet and copy in to buffer
		if(DEBUG_P)printf("%x\t", buf_n);
		fflush(stdout);
		
		if(timeout_sec > 0){
			tv.tv_sec = 0; tv.tv_usec = 0;
			while(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval)) < 0){
				fprintf(stderr, "setsockopt failed");
				sleep(1);
			}
		}
		
		// timed out
		if(buf_n < 1){
			return buf_n;
		}
		// received valid packet
		if(buf[0] == framing_bits){
			msg_start = 0; // starts at [0]
			//msg_start = frame_msg(buf, buf_n);
			ended = true;
		}
	}
	if(DEBUG_P)printf("\n");
	
	*msg_p = deserialize(&buf[msg_start+1], (buf_n-1) - (msg_start+1) +1);

	return buf_n;
}
/**
 * @brief Tries for a packet in raw socket, instant timeout
 * @param sock target socket
 * @param msg by reference
 * @param buf previously allocated buffer
 * @return 
 */
int try_packet(int sock, packet* msg_p, uint8_t* buf){
	struct sockaddr saddr;
	int saddr_len = sizeof(saddr);
	
	memset(buf, 0, BUF_MAX);
	
	int buf_n = recvfrom(sock, buf, BUF_MAX, MSG_DONTWAIT, &saddr, (socklen_t *)&saddr_len);
	// receive a network packet and copy in to buffer
	
	if(buf[0] != framing_bits){
		return 0;
	}

	*msg_p = deserialize(&buf[1], buf_n-1);

	return buf_n;
}
/**
 * @brief Sends message to socket
 * @param buf needs to be previously allocated
 * @return send() return value
 */
int send_msg(int sock, packet msg, uint8_t* buf){
	int buf_n = serialize(msg, buf);
	return send(sock, buf, buf_n, 0);
}
/**
 * @brief makes a string of command from msg
 * @param command is allocated memory
 * @return false if its not a command
 */
bool msg_to_command(packet msg, char** command){
	char* target = (char*)msg.data_p;
	switch(msg.type){
		case cd:
			*command = concat_to("cd ", target);
			break;
		case ls:
			*command = concat_to("ls ", target);
			break;
		case get:
			*command = concat_to("get ", target);
			break;
		case put:
			*command = concat_to("put ", target);
			break;
		default:
			return false;
	}
	return true;
}

/**
 * @brief prints how numbers are stored in memory as bytes
 */
void endian_test(){
	packet msg;
	msg.data_p = NULL;
	unsigned long long seq = 1;
	while(seq <= ((unsigned long long)0x1 << 32)){
		free(msg.data_p);
		msg.data_p = NULL;
		msg.size = floor(log2(seq)) + 1;
		msg.data_p = (uint8_t*)malloc(data_max);
		memset(msg.data_p, 0, data_max);
		*(ulong*)msg.data_p = seq;
		for(int i = 0; i < (int)(32/8); i++){
			if(msg.data_p[i] != 0){
				printf("%02x ", msg.data_p[i]);
			}else{
				printf("   ");
			
			}
		}
		printf("\n");
		
		unsigned long long test = *(ulong*)msg.data_p;
		printf("%llu\n", test);
		
		seq <<= 1;
	}
}
/**
 * @brief reads a message from stdin, prompting in stdout
 */
void read_msg(packet* msg){
	printf("Receiving Packet ");
	int result;
	
	printf("error = ");
	if(scanf("%x", &result) < 0) fprintf(stderr, "scan err\n");
	msg->error = result;
	
	printf("enter seq = ");
	if(scanf("%x", &result) < 0) fprintf(stderr, "scan err\n");
	msg->seq = result;
	
	printf("enter size = ");
	if(scanf("%x", &result) < 0) fprintf(stderr, "scan err\n");
	msg->size = result;
	
	printf("enter type = ");
	if(scanf("%x", &result) < 0) fprintf(stderr, "scan err\n");
	msg->type = result;
	
	msg->data_p = (uint8_t*)malloc(msg->size);
	printf("enter data = ");
	for(int i = 0; i < msg->size; i++){
		if(scanf("%x", &result) < 0) fprintf(stderr, "scan err\n");
		msg->data_p[i] = result;
	}
}

#endif