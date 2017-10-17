#ifndef WINDOW_H
#define WINDOW_H

#include <math.h>

#include "Protocol.h"

#define DEBUG_W true

#define window_size 3
#define w_mod(X) mod((X),(window_size))

typedef struct {
	packet arr[window_size];
	int start;
	// index of the last msg without error, starting from 'start'
	int acc;
} Window;

// index of last element; by definition w_mod(start -1)
int w_end(Window* this){
	return w_mod(this->start -1);
}

packet w_back(Window* this){
	return this->arr[w_end(this)];
}

void w_init(Window* this, int seq){
	this->acc = -1;
	this->start = 0;
	int i = 0;
	do {
		this->arr[w_mod(this->start +i)].type = invalid;
		this->arr[w_mod(this->start +i)].data_p = NULL;
		this->arr[w_mod(this->start +i)].seq = seq_mod(seq + i);
		this->arr[w_mod(this->start +i)].error = true;
		
		i += 1;
	} while(w_mod(this->start +i) != w_mod(w_end(this) +1));
}

packet w_front(Window* this){
	return this->arr[this->start];
}

int i_to_seq(Window* this, int i){
	return seq_mod(w_front(this).seq + w_mod(i - this->start));
}

packet last_acc(Window* this){
	packet acc_msg = this->arr[w_mod(this->acc)];
	acc_msg = this->arr[this->acc];
	// if acc is pointing to a not accepted msg, or no message is accepted (since the front is not)
	if(acc_msg.error || w_front(this).error){
		// return the msg seq from previous window shift (certainly accepted)
		acc_msg.seq = seq_mod(w_front(this).seq -1);
	}
	return acc_msg;
}

packet first_err(Window* this){
	return this->arr[w_mod(this->acc +1)];
}

int indexes_remain(Window* this){
	return seq_mod(w_back(this).seq - last_acc(this).seq);
}

typedef struct {
	int sock;
	uint8_t* buf; //to receive data from sock
	
	Window window;
	
	int rseq, sseq;
} Slider;

int seq_to_i(Window* this, int seq){
	// 0 1 2 3 4
	//   a s
	// 3 4 2
	// if I want index of packet.seq 5, given: start is 0;[0].seq == 3
	// 3 4 5 6 7	5-3 = 2		0 + 2 = 2
	return w_mod(this->start + (seq - this->arr[this->start].seq));
}

packet* at_seq(Window* this, int seq){
	return &this->arr[seq_to_i(this, seq)];
}

void slider_init(Slider* this, char* device){
	this->sock = raw_socket_connection(device);
	this->buf = (uint8_t*)malloc(BUF_MAX);
	
	this->rseq = 0;
	this->sseq = 0;
}

void set_seq(Slider* this, packet* msg){
	msg->seq = this->sseq;
	this->sseq = seq_mod(this->sseq +1);
}
/**
 * @brief sends msg with num in the data_p
 * doesn't increase sseq if this is a retry (we timed out)
 */
void send_number(Slider* this, msg_type_t typ, uint64_t num){
	packet msg;
	
	msg.type = typ;
	
	set_seq(this, &msg);
	
	set_data(&msg, num);
	
	send_msg(this->sock, msg, this->buf);
}

int sl_recv(Slider* this, packet* msg, int timeout_sec){
	int buf_n = rec_packet(this->sock, msg, this->buf, timeout_sec);
	if(buf_n < 1){
		if(DEBUG_W)printf("Timed out.\n");
		return buf_n;
	}
	
	while(seq_after(this->rseq, msg->seq)){
		if(DEBUG_W)printf("msg.seq too low, this->rseq=%hhx, discarted: ", this->rseq);
		if(DEBUG_W)print(*msg);
		rec_packet(this->sock, msg, this->buf, 0);
	}
	
	if(this->rseq != msg->seq){
		if(DEBUG_W)printf("\tExpected seq=%x received=%x\n", this->rseq, msg->seq);
		this->rseq = msg->seq;
	}
	
	this->rseq = seq_mod(this->rseq +1);
	
	return buf_n;
}
/**
 * @brief Sends message of current sequence
 */
void sl_send(Slider* this, packet* msg){
	set_seq(this, msg);
	
	if(DEBUG_W)printf("> Sending\n ");
	if(DEBUG_W)print(*msg);
	send_msg(this->sock, *msg, this->buf);
}
/**
 * @brief Repeats message until valid response arrives
 */
packet sl_talk(Slider* this, packet msg){
	packet response;
	bool responded = false;
	
	set_seq(this, &msg);
	
	if(DEBUG_W)printf("> Sending\n ");
	if(DEBUG_W)print(msg);
	while( ! responded){
		if(DEBUG_W)printf("try... ");
		send_msg(this->sock, msg, this->buf);
		
		// with timeout
		int buf_n = sl_recv(this, &response, TIMEOUT);
		if(buf_n < 1){
			if(DEBUG_W)printf("timeout \t");
			continue; // no response, send again
		}
		
		if(response.type == nack){
			if(*(uint64_t*)response.data_p != msg.seq){
				fprintf(stderr, "response nacked different sent seq\n");
			}
			continue; // send again
		}
		
		responded = true;
	}
	
	if(DEBUG_W)printf("< Response:\n");
	if(DEBUG_W)print(response);
	
	return response;
}

#endif