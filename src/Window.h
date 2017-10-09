#ifndef WINDOW_H
#define WINDOW_H

#include <math.h>

#include "Protocol.h"

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

/**
 * @brief sends ack "seq"
 * doesn't increase sseq if this is a retry (we timed out)
 */
void send_ack(Slider* this, uint8_t seq){
	static packet response;
	static int prev_seq = -1;
	
	if(prev_seq != seq){
		response.type = ack;
		
		response.seq = this->sseq;
		this->sseq = seq_mod(this->sseq +1);
		
		set_data(&response, seq);
	}
	
	send_msg(this->sock, response);
}

/**
 * @brief sends nack "seq"
 * doesn't increase sseq if this is a retry (we timed out)
 */
void send_nack(Slider* this, uint8_t seq){
	static packet response;
	static int prev_seq = -1;
	
	if(prev_seq != seq){
		response.type = nack;
		
		response.seq = this->sseq;
		this->sseq = seq_mod(this->sseq +1);
		
		set_data(&response, seq);
	}
	
	send_msg(this->sock, response);
}

packet sl_recv(Slider* this){
	packet msg;
	
	// block until at least 1 msg
	rec_packet(this->sock, &msg, this->buf, 0);
	
	if(msg.seq != this->rseq){
		fprintf(stderr, "received a msg with wrong seq\n");
	}
	
	this->rseq = seq_mod(this->rseq +1);
	
	return msg;
}

packet sl_send(Slider* this, packet msg){
	packet response;
	bool responded = false;
	
	msg.seq = this->sseq;
	this->sseq = seq_mod(this->sseq +1);
	
	printf("sending\n ");
	print(msg);
	while( ! responded){
		printf("again ");
		send_msg(this->sock, msg);
		
		// with timeout
		int buf_n = rec_packet(this->sock, &response, this->buf, 1);
		if(buf_n < 1){
			printf("timeout. \t");
			continue; // no response, send again
		}
		if(response.seq != this->rseq){
			fprintf(stderr, "received a response with wrong seq\n");
		}
		this->rseq = seq_mod(this->rseq +1);

		if(response.type == nack){
			if(*(uint64_t*)response.data_p != msg.seq){
				fprintf(stderr, "response nacked different sent seq\n");
			}
			continue; // send again
		}
		
		responded = true;
	}
	
	return response;
}

#endif