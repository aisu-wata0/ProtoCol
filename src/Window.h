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
		this->arr[w_mod(this->start +i)].seq = w_mod(seq + i);
		this->arr[w_mod(this->start +i)].error = true;
		
		i += 1;
	} while(w_mod(this->start +i) != w_mod(w_end(this) +1));
}

packet w_front(Window* this){
	return this->arr[this->start];
}

int i_to_seq(Window* this, int i){
	int ret = seq_mod(w_front(this).seq + i - this->start);
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

void print_slider(Slider* this){
	printf("indexes remain = %d; \tstart=%x; \tacc=%x; \n", indexes_remain(&this->window), this->window.start, this->window.acc);
	int it;
	
	it = this->window.start;
	do{
		printf(" %x", it);
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
	
	it = this->window.start;
	do{
		if(this->window.arr[it].error){
			printf("  ");
		} else {
			printf(" %x", this->window.arr[it].seq % 0xf);
		}
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
	
	it = this->window.start;
	do{
		printf(" %x", this->window.arr[it].type % 0xf);
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
	
	it = this->window.start;
	do{
		if(it == this->window.acc){
			printf(" a");
		} else {
			printf("  ");
		}
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
}

void print_window(Slider* this){
	printf("\n");
	printf("acc=%x; \tstart=%x; \n", this->window.acc, this->window.start);
	int it;
	
	it = this->window.start;
	do{
		printf(" %x", it);
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
	
	it = this->window.start;
	do{
		printf(" %x", this->window.arr[it].seq % 0xf);

		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
	
	it = this->window.start;
	do{
		if(this->window.arr[it].error){
			printf(" t"); // to send
		} else {
			printf(" s"); // sent
		}
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
	
	it = this->window.start;
	do{
		printf(" %x", this->window.arr[it].type % 0xf);
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
	
	it = this->window.start;
	do{
		if(it == this->window.acc){
			printf(" a");
		} else {
			printf("  ");
		}
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
}

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
 * @param seq
 */
void send_ack(Slider* this, uint8_t seq){
	static packet response;
	static int prev_seq = -1;
	
	if(prev_seq != seq){
		response.type = ack;
		
		response.seq = this->sseq;
		this->sseq = seq_mod(this->sseq +1);
		
		response.size = ceil(seq_b/(double)8);
		response.data_p = (uint8_t*)malloc(response.size);
		response.data_p[0] = seq;
	}
	
	send_msg(this, response);
}

/**
 * @brief sends nack "seq"
 */
void send_nack(Slider* this, uint8_t seq){
	static packet response;
	static int prev_seq = -1;
	
	if(prev_seq != seq){
		response.type = nack;
		
		response.seq = this->sseq;
		this->sseq = seq_mod(this->sseq +1);
		//response.size = floor(log2(seq)) + 1;
		response.size = ceil(seq_b/(double)8);
		response.data_p = (uint8_t*)malloc(response.size);
		response.data_p[0] = seq;
	}
	
	send_msg(this, response);
}

/**
 * @brief if there's packets waiting and space in the queue, push packets
 */
//void update_queue(Slider* this){
//	int result = 1;
//	packet msg;
//	
//	while(result > 0 && this->queue.size < window_size){
//		result = try_packet(this->sock, &msg, this->buf);
//		if(result > 0){
//			if(msg.error){
//				send_nack(msg.seq);
//			} else if(msg.seq == this->seq){
//				push_back(this->queue, msg);
//			} else if(msg.seq > this->seq){
//				// (msg.seq - back(this->queue).seq) == how much more space you need to store this message
//				// if msg.seq comes after the back packet it how need n more indexes, where n is how ahead is the msg
//				// eg: if the back elem is 2, you receive msg.seq = 4, you should need 2 more indexes
//				// [2,  , 4], where 3 will be invalid, now 4 is the back elem
//				// if the packet on the back has seq greater than msg.seq, it already should have space in the array
//				if( (window_size - this->queue.size) >= (msg.seq - back(this->queue).seq) ){
//					this->queue.arr[ (this->queue.acc + (msg.seq - this->seq)) % window_size ] = msg;
//					this->queue.acc + 1 + (msg.seq - this->seq) % window_size
//				}
//			}
//		}
//	}
//	
//	send_ack(this->seq -1);
//}
//
//packet sl_rec_packet(Slider* this){
//	packet msg;
//	int result;
//	
//	// block until at least 1 msg
//	if(this->queue.size == 0){
//		rec_packet(this->sock, &msg, this->buf, 0);
//		push_back(this->queue, msg);
//	}
//	
//	msg = w_front(this);
//	pop(this);
//	update_queue(this);
//	
//	return msg;
//}

#endif