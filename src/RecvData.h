#ifndef RECVDATA_H
#define RECVDATA_H

#include <math.h>

#include "Window.h"

void print_slider(Slider* this){
	printf("indexes remain = %x; \tstart=%x; \tacc=%x; \n", indexes_remain(&this->window), this->window.start, this->window.acc);
	int it;
	
	printf("indexes  ");
	it = this->window.start;
	do{
		printf("  \t%x", it);
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
	
	printf("sequence ");
	it = this->window.start;
	do{
		printf(" \t");
		if(this->window.arr[it].error){
			printf("-");
		} else {
			printf("+");
		}
		printf("%x", this->window.arr[it].seq);
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
	
	printf("type     ");
	it = this->window.start;
	do{
		printf(" \t%x", this->window.arr[it].type);
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
	
	printf("pointers ");
	it = this->window.start;
	do{
		printf(" \t");
		if(it == this->window.acc){
			printf("a");
		} else if(it == this->window.start) {
			printf("s");
		} else {
			printf(" ");
		}
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
}
/**
 * @brief if proper, adds message to window and increments window.acc
 * @param msg 
 * @return true if a message was added to the window
 */
int handle_msg(Slider* this, packet msg){
	if(seq_after(msg.seq, w_back(&this->window).seq)){
		fprintf(stderr, "received msg ahead of the window\n");
		msg.error = true;
	}
	if(seq_after(w_front(&this->window).seq, msg.seq)){
		fprintf(stderr, "received msg before window\n");
		msg.error = true;
	}
	
	int msg_pos = seq_to_i(&this->window, msg.seq);
	// if msg with error, or already received this seq
	if(msg.error || !this->window.arr[msg_pos].error){
		if(DEBUG_W) printf("message not added\n");
		if(DEBUG_W) if(!this->window.arr[msg_pos].error) printf("message was already recvd\n");
		if(DEBUG_W) if(msg.data_p == this->window.arr[msg_pos].data_p) printf("WARN: Message duplicate\n");
		// free only if not duplicate pointer
		if(msg.data_p != this->window.arr[msg_pos].data_p)
			free(msg.data_p);
		msg.data_p = NULL;
		
		return false;
	}
	// insert message
	this->window.arr[msg_pos] = msg;
	// update which is the last message received without errors in the sequence
	while( (indexes_remain(&this->window) > 0) && !(this->window.arr[w_mod(this->window.acc + 1)].error) ){
	// [0 _ 2 3 4], acc == 0, receive 1
	// [0 1 2 3 4], acc == 4 == w_end
		this->window.acc = w_mod(this->window.acc +1);
	}
	if(DEBUG_W)printf("added msg to window, new acc=%x, last_acc.seq=%x\n", this->window.acc, last_acc(&this->window).seq);
	
	return true;
}
/**
 * @brief writes data from received accepted messages to file stream
 * @param rec_size is incremented by the size of the msgs written
 * @param ended setted to true if reached file end msg
 */
void write_to_file(Slider* this, FILE* stream, long long* rec_size, bool* ended){
	int msgs_to_write = seq_mod( last_acc(&this->window).seq +1 - w_front(&this->window).seq );
	// has at least one msg to write
	if( msgs_to_write > 0 ){
		if(DEBUG_W)printf("writing seqs: [ \n");
		int it = this->window.start;
		do {
			if(this->window.arr[it].type == end){
				*ended = true;
				break;
			}
			if(DEBUG_W)printf("%hx, ", this->window.arr[it].seq);
			// writing data to file
			//fwrite(this->window.arr[it].data_p, 1, this->window.arr[it].size, stream);
			fwrite(this->window.arr[it].data_p, this->window.arr[it].size, 1, stream);
			
			if(DEBUG_W)printf("addr = %lx; ", (uint64_t)this->window.arr[it].data_p);
			if(DEBUG_W)printf("data = %s\n", (char*)this->window.arr[it].data_p);
			*rec_size += this->window.arr[it].size;
			
			it = w_mod(it + 1);
		} while(it != w_mod(this->window.acc +1));
		if(DEBUG_W)printf("]\n");
	}
}
/**
 * @brief gives apropriate response to sender about the current window
 */
packet response(Slider* this){
	packet msg = NIL_MSG;
	if(DEBUG_W)printf("Response ");
	if((w_back(&this->window).seq == last_acc(&this->window).seq) || (last_acc(&this->window).type == end)){
		if(DEBUG_W)printf("ack %x\n", last_acc(&this->window).seq);
		msg.type = ack;
		set_data(&msg, last_acc(&this->window).seq);
	} else {
		if(DEBUG_W)printf("nack %x\n", first_err(&this->window).seq);
		msg.type = nack;
		set_data(&msg, first_err(&this->window).seq);
	}
	return msg;
}
/**
 * @brief moves window to start after last accepted message
 */
void move_window(Slider* this){
	// 3 4 5 6	front is [0]=3, acc is [2]=5
	// 7 8 9 6	[0]=5+1 [1]=5+2 [2]=5+3
	//       s
	// 3 4 5 6	front is [0]=3, acc is [2]=5
	// d b a(d
	// 0 1 2 0
	// b)b a b
	// move window only if the first has been ack
	if(!w_front(&this->window).error){
		int i = 1;
		int it = this->window.start;
		do{
			this->window.arr[it].seq = seq_mod(w_back(&this->window).seq + i);
			// last it of loop will be w_end
			this->window.arr[it].type = invalid;
			this->window.arr[it].error = true;
			
			free(this->window.arr[it].data_p);
			this->window.arr[it].data_p = NULL;
			
			i += 1;
			it = w_mod(it+1);
		} while (it != w_mod(this->window.acc +1));
		this->window.start = w_mod(this->window.acc +1);
	}
}
/**
 * @brief Sends ok message, receives data and writes it to file stream, stops when rec_size >= data_size
 * @param stream to write data into
 * @param data_size to receive
 * @return rec_size: size of the data received
 */
long long receive_data(Slider* this, FILE* stream){
	packet msg;
	packet resp = NIL_MSG;
	bool ended = false;
	long long rec_size = 0;
	w_init(&this->window, this->rseq);
	
	int buf_size;
	
	msg = NIL_MSG;
	msg.type = ok;
	/**/
	msg = talk(this, msg, 0);
	/*DEBUG
	read_msg(&msg);
	*/
	if(DEBUG_W)print(msg);
	if( ! isData(msg)){
		errno = (*(int*)msg.data_p);
		return rec_size;
	}
	handle_msg(this, msg);
	
	if(DEBUG_W)print_slider(this);
	
	while(!ended){
		// wait first packet of stream
		buf_size = rec_packet(this->sock, &msg, this->buf, 0);
		// while there's packets on stream
		while(buf_size > 0){
			if(DEBUG_W)print(msg);
			if( ! isData(msg)){
				this->rseq = seq_mod(last_acc(&this->window).seq +1);
				return rec_size;
			}
			handle_msg(this, msg);
			
			if(DEBUG_W)print_slider(this);
			
			// try another packet of the stream
			/**/
			buf_size = 0;
			if(indexes_remain(&this->window) > 0){
				buf_size = try_packet(this->sock, &msg, this->buf);
			}
			/*DEBUG*
			printf("next msg buf_size = ");
			int result;
			if(scanf("%x", &result) < 0) fprintf(stderr, "scan error\n");
			buf_size = result;
			/**/
		}
		
		write_to_file(this, stream, &rec_size, &ended);
		
		printf("Current recv bytes: %h\n", rec_size);
		
		resp = response(this);
		if(last_acc(&this->window).type != end){
			set_seq(this, &resp);
			if(DEBUG_W) printf("set_seq to %h", resp.seq);
			send_msg(this->sock, resp, this->buf);
		}
		
		move_window(this);
		
		if(DEBUG_W)print_slider(this);
	}
	
	this->rseq = seq_mod(last_acc(&this->window).seq +1);
	
	say(this, resp);
	
	return rec_size;
}


#endif