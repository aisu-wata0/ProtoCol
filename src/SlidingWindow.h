#ifndef SLIDINGWINDOW_H
#define SLIDINGWINDOW_H

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
		this->arr[w_mod(this->start +i)].type = ok;
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

packet last_ok(Window* this){
	packet ret = this->arr[w_mod(this->acc)];
	ret = this->arr[this->acc];
	if(ret.error || w_front(this).error){
		ret.seq = seq_mod(w_front(this).seq -1);
	}
	return ret;
}

packet first_err(Window* this){
	packet ret = this->arr[w_mod(this->acc +1)];
	return this->arr[w_mod(this->acc +1)];
}

int indexes_remain(Window* this){
	int ret = seq_mod(w_back(this).seq - last_ok(this).seq);
	if(ret == 0 && last_ok(this).error){
		ret = 3;
	}
	return ret;
}

//int pop(Window* this){
//	if(this->size > 0){
//		this->start = (this->start + 1) % window_size;
//		this->size -= 1;
//		return this->size;
//	} else {
//		return FAIL;
//	}
//}
//
//int push_back(Window* this, packet msg){
//	if(size != window_size){
//		this->arr[this->end] = msg;
//		this->end = (this->end + 1) % window_size;
//		this->size += 1;
//		return this->size;
//	} else {
//		return FAIL;
//	}
//}

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

void sl_send_msg(Slider* this, packet msg){
	this->sseq = seq_mod(this->sseq +1);
	
	send_msg(this->sock, msg);
}

void send_ack(Slider* this, uint8_t seq){
	packet response;
	response.size = ceil(seq_b/(double)8);
	
	response.seq = this->sseq;
	
	response.type = ack;
	response.data_p = (uint8_t*)malloc(response.size);
	response.data_p[0] = seq;
	
	sl_send_msg(this, response);
}

void send_nack(Slider* this, uint8_t seq){
	packet response;
	response.size = ceil(seq_b/(double)8);
	
	response.seq = this->sseq;
	
	response.type = nack;
	response.data_p = (uint8_t*)malloc(response.size);
	response.data_p[0] = seq;
	
	sl_send_msg(this, response);
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
//packet rec_packet(Slider* this){
//	packet msg;
//	int result;
//	
//	// block until at least 1 msg
//	if(this->queue.size == 0){
//		rec_packet(this->sock, &msg, this->buf);
//		push_back(this->queue, msg);
//	}
//	
//	msg = w_front(this);
//	pop(this);
//	update_queue(this);
//	
//	return msg;
//}


int handle_msg(Slider* this, packet msg){
	int new_msg = FAIL;
	int okseq = last_ok(&this->window).seq;
	int ahead = seq_mod(msg.seq - okseq);
	
	if( !msg.error && (ahead > 0) ){
		int msg_pos = seq_to_i(&this->window, msg.seq);
		// if hadn't received this msg yet
		if(this->window.arr[msg_pos].error){
			new_msg = true;
			this->window.arr[msg_pos] = msg;
			// update which is the last message received without errors in the sequence
			while( (indexes_remain(&this->window) > 0) && !(this->window.arr[w_mod(this->window.acc + 1)].error) ){
			// [0 _ 2 3 4], acc == 0, receive 1
			// [0 1 2 3 4], acc == 4 == w_end
				this->window.acc = w_mod(this->window.acc +1);
			}
			
			return true;
		}
	}
	
	free(msg.data_p);
	msg.data_p = NULL;
	
	return false;
}

void respond(Slider* this){
	// DEBUG
	printf("response ");
	int one = i_to_seq(&this->window, w_end(&this->window));
	int two = w_back(&this->window).seq;
	int last_seq = last_ok(&this->window).seq;
	if(w_back(&this->window).seq == last_ok(&this->window).seq){
		printf("ack %x\n", last_ok(&this->window).seq);
		send_ack(this, last_ok(&this->window).seq);
	} else {
		printf("nack %x\n", first_err(&this->window).seq);
		send_nack(this, first_err(&this->window).seq);
	}
	// 3 4 5 6	front is [0]=3, acc is [2]=5
	// 7 8 9 6	[0]=5+1 [1]=5+2 [2]=5+3
	// move window only if the first has been ack
	if(!last_ok(&this->window).error){
		int i = 1;
		int it = w_mod(this->window.acc +1);
		do{
			this->window.arr[it].seq = seq_mod(last_ok(&this->window).seq + i);
			// last it of loop will be w_end
			this->window.arr[it].type = ok;
			this->window.arr[it].error = true;
			
			free(this->window.arr[it].data_p);
			this->window.arr[it].data_p = NULL;
			
			i += 1;
			it = w_mod(it+1);
		} while (it != w_mod(this->window.acc +1));
		this->window.start = w_mod(this->window.acc +1);
	}
	// DEBUG
	print_slider(this);
}

int receive_data(Slider* this, FILE* stream, int data_size){
	packet msg;
	int rec_size = 0;
	w_init(&this->window, this->rseq);
	
	int buf_size;

	while(rec_size < data_size){
		// DEBUG
		// buf_size = rec_packet(this->sock, &msg, this->buff);
		buf_size = 1;
		sleep(1);
		while(buf_size > 0){
		//while( indexes_remain(&this->window) > 0 ){
			// block until at least 1 msg
			// DEBUG
			// rec_packet(this->sock, &msg, this->buf);
			printf("Receiving Packet ");
			printf("error = ");
			int result;
			if(scanf("%d", &result) < 0) fprintf(stderr, "scan error\n");
			msg.error = result;
			printf("enter seq = ");
			if(scanf("%x", &result) < 0) fprintf(stderr, "scan error\n");
			msg.seq = result;
			printf("enter size = ");
			if(scanf("%x", &result) < 0) fprintf(stderr, "scan error\n");
			msg.size = result;
			printf("enter type = ");
			if(scanf("%x", &result) < 0) fprintf(stderr, "scan error\n");
			msg.type = result;
			msg.data_p = (uint8_t*)malloc(msg.size);
			for(int i = 0; i < msg.size; i++){
				msg.data_p[i] = i;
			}
			msg.parity = parity(msg) + error;
			print(msg);
			
			handle_msg(this, msg);
			print_slider(this);
			
			// DEBUG
//			buf_size = 0;
//			if(indexes_remain(this->window) > 0){
//				buf_size = try_packet(this->sock, &msg, this->buf);
//			}
			printf("next msg buf_size = ");
			if(scanf("%x", &result) < 0) fprintf(stderr, "scan error\n");
			buf_size = result;
		}
		
		int msgs_to_write = seq_mod( last_ok(&this->window).seq -(w_front(&this->window).seq -1) );
		// has at least one msg to write
		if( msgs_to_write > 0 ){
			printf("started writing, seqs: ");
			int it = this->window.start;
			do {
				printf("%hx ", this->window.arr[it].seq % 0xf);
				fwrite(this->window.arr[it].data_p, 1, this->window.arr[it].size, stream);
				rec_size += this->window.arr[it].size;
				
				it = w_mod(it + 1);
			} while(it != w_mod(this->window.acc +1));
			printf("\n");
		}
		
		respond(this);
	}
	
	this->rseq = seq_mod(last_ok(&this->window).seq +1);
	return rec_size;
}

packet next_packet(Slider* this, FILE* stream){
	packet msg;
	msg.error = true;
	msg.seq = this->sseq;
	this->sseq = seq_mod(this->sseq +1);
	
	msg.size = fread(this->buf, 1, data_max, stream);
	if(msg.size > 0){
		msg.type = data;
		msg.data_p = (uint8_t*)malloc(msg.size);
		memcpy(msg.data_p, this->buf, msg.size);
	} else {
		printf("msg.size = fread returned %x bytes, assumed eof", msg.size);
		msg.type = end;
		msg.data_p = NULL;
	}
	return msg;
}

/**
 * @brief fill window from to w_end, from always gets filled
 */
void fill_window(Slider* this, int from, FILE* stream, int* ended){
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
		this->window.arr[i] = next_packet(this, stream);
		// DEBUG
		if(this->window.arr[i].seq != i_to_seq(&this->window, i)){
			printf("ERROR: seqs should be equal! %x != %x", this->window.arr[i].seq%0xf, i_to_seq(&this->window, i)%0xf);
			this->window.arr[i].seq = i_to_seq(&this->window, i);
		}
		
		if(this->window.arr[i].type == end){
			break;
		}
		
		i = w_mod(i+1);
	} while (i != w_mod(w_end(&this->window) +1));
}


void send_window(Window* this){
	printf("Sending window\n");
	int it = this->start;
	do {
		if(this->arr[it].error){
			this->arr[it].error = false;
			print(this->arr[it]);
			// DEBUG
			// send_msg(this->sock, this->arr[it]);
		}
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(this) +1));
}

int handle_response(Window* this, packet response, int* prev_start){
	*prev_start = this->start;
	int response_seq = response.data_p[0];
	
	switch(response.type){
		case nack:
			at_seq(this, response_seq)->error = true;
			// 3 4 5 6	receives nack 5, given: start = 0; [0].seq == 3
			// 7 8 5 6	start = 2; from 0 to <start, new packets into window
			this->start = seq_to_i(this, response_seq);
			// 3 4 5 6	receives nack 3; start = 0; no new packets
			if(*prev_start == this->start){
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
	}
	
	return true;
}

void send_data(Slider* this, FILE* stream){
	packet response;
	int prev_start = this->window.start;
	int ended = false;
	int fill = true;
	
	while(!ended){
		if(fill){
			fill_window(this, prev_start, stream, &ended);
		}
		fill = true;
		
		send_window(&this->window);
		
		printf("Receive reply? ");
		int reply;
		if(scanf("%d", &reply) < 0) fprintf(stderr, "scan error\n");
		if(reply > 0){
			continue;
		}
		// with timeout
//		if(rec_packet(int sock, packet* msg_p, uint8_t* buf) == FAIL){
//			continue;
//		}
		printf("enter seq = ");
		int result;
		if(scanf("%x", &result) < 0) fprintf(stderr, "scan error\n");
		response.seq = result;
		printf("enter size = ");
		if(scanf("%x", &result) < 0) fprintf(stderr, "scan error\n");
		response.size = result;
		response.data_p = (uint8_t*)malloc(response.size);
		for(int i = 0; i < response.size; i++){
			response.data_p[i] = i;
		}
		response.parity = parity(response);
		print(response);
		
		fill = handle_response(&this->window, response, &prev_start);
	}
	
	this->sseq = seq_mod(last_ok(&this->window).seq +1);
}

#endif