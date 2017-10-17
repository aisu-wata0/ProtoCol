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
		printf(" %x", it);
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
	
	printf("sequence ");
	it = this->window.start;
	do{
		if(this->window.arr[it].error){
			printf("-");
		} else {
			printf("+");
		}
		printf("%x", this->window.arr[it].seq % 0xf);
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
	
	printf("type     ");
	it = this->window.start;
	do{
		printf(" %x", this->window.arr[it].type % 0xf);
		
		it = w_mod(it+1);
	} while(it != w_mod(w_end(&this->window) +1));
	printf("\n");
	
	printf("pointers ");
	it = this->window.start;
	do{
		if(it == this->window.acc){
			printf(" a");
		} else if(it == this->window.start) {
			printf(" s");
		} else {
			printf("  ");
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
	
	if( !msg.error ){
		if( ! (msg.type == data || msg.type == end)){
			fprintf(stderr, "received a message of wrong type (%x) on data transfer\n", msg.type);
		}
		int msg_pos = seq_to_i(&this->window, msg.seq);
		// if hadn't received this msg yet
		if(this->window.arr[msg_pos].error){
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
		if(DEBUG_W)printf("had already received this message\n");
		if(msg.data_p == this->window.arr[msg_pos].data_p){
			msg.data_p = NULL;
		}
	}
	
	if(DEBUG_W)printf("message not added\n");
	
	free(msg.data_p);
	msg.data_p = NULL;
	
	return false;
}
/**
 * @brief writes data from received accepted messages to file stream
 * @param rec_size is incremented by the size of the msgs written
 * @param ended setted to true if reached file end msg
 */
void write_to_file(Slider* this, FILE* stream, uint64_t* rec_size, bool* ended){
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
			if(DEBUG_W)printf("%hx, ", this->window.arr[it].seq % 0xf);
			// writing data to file
			//fwrite(this->window.arr[it].data_p, 1, this->window.arr[it].size, stream);
			fwrite(this->window.arr[it].data_p, this->window.arr[it].size, 1, stream);
			
			if(DEBUG_W)printf("addr = %lx\n", (uint64_t)this->window.arr[it].data_p);
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
void respond(Slider* this){
	if(DEBUG_W)printf("Response ");
	if((w_back(&this->window).seq == last_acc(&this->window).seq) || (last_acc(&this->window).type == end)){
		if(DEBUG_W)printf("ack %x\n", last_acc(&this->window).seq);
		send_number(this, ack, last_acc(&this->window).seq);
	} else {
		if(DEBUG_W)printf("nack %x\n", first_err(&this->window).seq);
		send_number(this, nack, first_err(&this->window).seq);
	}
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
uint64_t receive_data(Slider* this, FILE* stream, uint64_t data_size){
	packet msg;
	bool ended = false;
	uint64_t rec_size = 0;
	w_init(&this->window, this->rseq);
	
	int buf_size;
	
	msg.type = ok;
	msg.size = 0;
	
	/*DEBUG*/
	msg = sl_talk(this, msg);
	/**
	read_msg(&msg);
	/**/
	
	buf_size = msg.size;
	
	if(msg.type != data){
		fprintf(stderr, "received wrong type on response\n");
	}

	if(DEBUG_W)print_slider(this);
	
	while(!ended){
		/*DEBUG*/
		if(msg.type == invalid){
			buf_size = rec_packet(this->sock, &msg, this->buf, 0);
		}
		/**
		buf_size = 1;
		/**/
		while(buf_size > 0){
			/*DEBUG*
			read_msg(&msg);
			/**/
			
			if(DEBUG_W)print(msg);
			
			handle_msg(this, msg);
			msg.type = invalid;
			
			if(DEBUG_W)print_slider(this);
			
			/*DEBUG*/
			buf_size = 0;
			if(indexes_remain(&this->window) > 0){
				buf_size = try_packet(this->sock, &msg, this->buf);
			}
			/**
			printf("next msg buf_size = ");
			int result;
			if(scanf("%x", &result) < 0) fprintf(stderr, "scan error\n");
			buf_size = result;
			/**/
		}
		
		write_to_file(this, stream, &rec_size, &ended);
		
		respond(this);
		
		move_window(this);
		
		if(DEBUG_W)print_slider(this);
	}
	if(rec_size < data_size){
		fprintf(stderr, "File transfer error, received %lu/%lu bytes", rec_size, data_size);
	}
	
	this->rseq = seq_mod(last_acc(&this->window).seq +1);
	return rec_size;
}


#endif