#ifndef SENDDATA_H
#define SENDDATA_H

#include <math.h>

#include "Window.h"

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

packet next_packet(Slider* this, FILE* stream){
	packet msg;
	msg.error = true;
	msg.seq = this->sseq;
	this->sseq = seq_mod(this->sseq +1);
	
	msg.data_p = (uint8_t*)malloc(data_max);
	msg.size = fread(msg.data_p, 1, data_max, stream);
	
	if(msg.size > 0){
		msg.type = data;
	} else {
		printf("msg.size = fread returned %x bytes, assumed eof\n", msg.size);
		msg.type = end;
		msg.data_p = NULL;
	}
	return msg;
}

/**
 * @brief fill window from to w_end, from always gets filled
 */
void fill_window(Slider* this, int from, FILE* stream, bool* ended){
	static bool eof = false;
	// 3 4 5 6	receives nack 5, given: start = 0; [0].seq == 3
	// 7 8 5 6	start = 2; from 0 to <start, new packets into window
	int i = from;
	do {
		// handle old msg
		free(this->window.arr[i].data_p);
		this->window.arr[i].data_p = NULL;
		if(this->window.arr[i].type == end){
			*ended = true;
			break;
		}
		// queue next
		if( ! eof){
			this->window.arr[i] = next_packet(this, stream);
			if(this->window.arr[i].type == end){
				eof = true;
			}
		} else { // no message to fill in
			this->window.arr[i].type = invalid;
			this->window.arr[i].seq = i_to_seq(&this->window, i);
		}
		// DEBUG
		if(this->window.arr[i].seq != i_to_seq(&this->window, i)){
			printf("ERROR: seqs should be equal! %x != %x\n", this->window.arr[i].seq%0xf, i_to_seq(&this->window, i)%0xf);
			this->window.arr[i].seq = i_to_seq(&this->window, i);
		}
		
		i = w_mod(i+1);
	} while (i != w_mod(w_end(&this->window) +1));
}

void set_sent(Window* this){
	printf("setting sent >\n");
	int it = this->start;
	do {
		if(this->arr[it].type == invalid){
			break;
		}
		if(this->arr[it].error){
			this->arr[it].error = false;
		}
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(this) +1));
}

void send_window(Slider* this){
	printf("Sending window >\n");
	int it = this->window.start;
	do {
		if(this->window.arr[it].type == invalid){
			break;
		}
		if(this->window.arr[it].error){
			print(this->window.arr[it]);
			send_msg(this->sock, this->window.arr[it]);
		}
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("> sent\n\n");
}

int handle_response(Window* this, packet response){
	this->acc = this->start;
	int response_seq = response.data_p[0];
	
	switch(response.type){
		case nack:
			at_seq(this, response_seq)->error = true;
			// 3 4 5 6	receives nack 5, given: start = 0; [0].seq == 3
			// 7 8 5 6	start = 2; from 0 to <start, new packets into window
			this->start = seq_to_i(this, response_seq);
			// 3 4 5 6	receives nack 3; start = 0; no new packets
			if(this->acc == this->start){
				return false;
			}
			break;
		case ack:
			
			// 3 4 5 6	receives ack 5, given: start = 0; [0].seq == 3
			// 7 8 9 6	start = 3; from 0 to <start, new packets into window
			// 7 8 9 A	receives ack 6;	start = 0; from 0 to <0
			this->start = w_mod(seq_to_i(this, response_seq) +1);
			break;
		default:
			fprintf(stderr, "received a response that isn't ack nor nack in data transfer\n");
			return false;
	}
	
	return true;
}

void send_data(Slider* this, FILE* stream){
	packet response;
	w_init(&this->window, this->rseq);
	
	this->window.acc = this->window.start;
	
	bool fill;
	bool ended = false;
	// startup
	fill_window(this, this->window.acc, stream, &ended);
	
	while(!ended){
		print_window(this);
		
		send_window(this);
		// DEBUG
		// with timeout
		int buf_n = rec_packet(this->sock, &response, this->buf, 1);
		if(buf_n < 1){
			printf("Timed out\n");
			continue; // no response, send window again
		}
//		printf("Receive reply? ");
//		int reply;
//		if(scanf("%d", &reply) < 0) fprintf(stderr, "scan error\n");
//		if(reply < 1){
//			continue;
//		}
//		int result;
//		
//		printf("enter seq = ");
//		if(scanf("%x", &result) < 0) fprintf(stderr, "scan error\n");
//		response.seq = result;
//		
//		printf("enter type = ");
//		if(scanf("%x", &result) < 0) fprintf(stderr, "scan error\n");
//		response.type = result;
//		
//		response.size = 1;
//		response.data_p = (uint8_t*)malloc(1);
//		printf("data_p[0] = ");
//		if(scanf("%x", &result) < 0) fprintf(stderr, "scan error\n");
//		response.data_p[0] = result;
//		
//		printf("error = ");
//		if(scanf("%x", &result) < 0) fprintf(stderr, "scan error\n");
//		response.error = result;

		set_sent(&this->window);
		
		printf("handling response\n");
		print(response);
		
		// TODO check response.seq or use sl_recv
		
		fill = handle_response(&this->window, response);
		
		if(fill){
			fill_window(this, this->window.acc, stream, &ended);
		}
	}
	
	/*DEBUG*/
	if(this->sseq != seq_mod(w_back(&this->window).seq +1)){
		printf("ERROR: seqs should be equal! %x != %x\n", this->sseq%0xf, seq_mod(w_back(&this->window).seq +1)%0xf);
	}/**/
}

#endif