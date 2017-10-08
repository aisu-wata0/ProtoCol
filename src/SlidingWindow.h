#ifndef SLIDINGWINDOW_H
#define SLIDINGWINDOW_H

#define window_size 3
#define mod(X) (X % window_size)

typedef struct {
	packet arr[window_size];
	int start;
	// index of the last msg without error, starting from 'start'
	int acc;
	// index of the highest received msg seq
	int hseq;
} Window;

// end index is by definition mod(start -1)
void q_end(Window* this){
	return mod(this->start -1);
}

void q_init(Window* this, int last_received_seq){
	this->start = 0;
	this->acc = -1;
	this->hseq = last_received_seq;
	int i = this->start;
	do {
		this->arr[i].error = true;
		
		i = mod(i+1);
	} while(i != this->start);
}

packet front(Window* this){
	return this->arr[this->start];
}

packet last_ok(Window* this){
	return this->arr[this->acc];
}

packet first_err(Window* this){
	return this->window.arr[mod(this->window.acc +1)];
}

int indexes_remain(Window* this){
	return mod(q_end(this) - this->acc);
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
	
	int seq;
} Slider;

void print_slider(Slider* this){
	for(int i = 0; i < window_size; i++){
		printf(" %x", i % 0xf);
	}
	
	int i;
	i = this->window.start;
	do{
		if(msg.error){
			printf("  ");
		} else {
			printf(" %x", this->window.arr[i].seq % 0xf);
		}
		
		i = mod(i+1);
	} while(i != mod(q_end(this->window) +1));
	
	i = this->window.start;
	do{
		if(i == this->window.acc){
			printf(" a");
		} else {
			printf("  ");
		}
		
		i = mod(i+1);
	} while(i != mod(q_end(this->window) +1));
}

int seq_to_i(Window* this, int seq){
	// if I want index of packet.seq 5, given: start is 0;[0].seq == 3
	// 3 4 5 6 7	5-3 = 2		0 + 2 = 2
	return this->start + (seq - this->arr[this->start].seq);
}

packet at_seq(Window* this, int seq){
	return this->arr[seq_to_i(this, seq)];
}

void slider_init(Slider* this, char* device){
	this->sock = raw_socket_connection(device);
	this->buf = (uint8_t*)malloc(BUF_MAX);
	
	this->seq = 0;
}

void sl_send_msg(Slider* this, packet msg){
	this->sseq += 1;
	
	send_msg(sock, response);
}

void send_nack(Slider* this, uint8_t nack_seq){
	packet response;
	response.size = 1;
	response.seq = this->seq;
	response.type = nack;
	response.data_p = (uint8_t*)malloc(ceil(seq_b/(double)8));
	response.data_p[0] = nack_seq;
	
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
//	msg = front(this);
//	pop(this);
//	update_queue(this);
//	
//	return msg;
//}


int handle_msg(Slider* this, packet msg){
	if(msg.seq > this->window.hseq){
		this->window.hseq = msg.seq;
	}
	if(!msg.error && (msg.seq > this->window.acc) ){
		int msg_pos = seq_to_i(this->window, msg.seq);
		// if hadn't received this msg yet
		if(this->window.arr[msg_pos].error){
			this->window.arr[msg_pos] = msg;
			// update which is the last message received without errors in the sequence
			while( (indexes_remain(this->window) > 0) && (!this->window.arr[mod(this->window.acc + 1)].error) ){
			// [0 _ 2 3 4], acc == 0, receive 1
			// [0 1 2 3 4], acc == 4 == q_end
				this->window.acc = mod(this->window.acc +1);
				seq += 1;
			}
		} else {
			free(this->window.arr[msg_pos].data_p);
		}
	}
	
	return this->window.acc;
}

void respond(Slider* this){
	if(this->window.hseq == this->window.acc){
		send_ack(last_ok(this->window).seq);
	} else {
		send_nack(fist_err(this->window).seq);
	}
	for(int i = this->window.start; i != mod(this->window.acc +1);){
		free(this->window.arr[i].data_p);
		this->window.arr[i].error = true;
		
		i = mod(i+1);
	}
	this->window.start = mod(this->window.acc +1);
}

int receive_data(Slider* this, FILE* stream, int data_size){
	int rec_size = 0;
	q_init(&this->window, this->seq -1);
	
	int buf_size;

	while(rec_size != data_size){
		// buf_size = rec_packet(this->sock, &msg, this->buff);
		// while(buf_size > 0){
		while( indexes_remain(this->window) > 0 && (  this->window.acc != q_end(this->window.end) )){
			// block until at least 1 msg
			// rec_packet(this->sock, &msg, this->buf);
			printf("Receiving Packet ");
			printf("enter seq = ");
			scanf("%d", msg.seq);
			printf("enter size = ");
			scanf("%d", msg.size);
			msg.data_p = (uint8_t*)malloc(msg.size);
			for(int i = 0; i < msg.size; i++){
				msg.data_p[i] = i;
			}
			msg.parity = parity(msg);
			print(msg);
			
			int i = mod(this->window.acc + 1);
			handle_msg(this, msg);
			i = 
			for(; i != mod(this->window.acc +1);){
				fwrite(this->window.arr[i].data_p, 1, this->window.arr[i].size, stream);
				rec_size += this->window.arr[i].size;
				
				int i = mod(i + 1);
			}
			
//			buf_size = 0;
//			if(indexes_remain(this->window) > 0){
//				buf_size = try_packet(this->sock, &msg, this->buf);
//			}
		}
		respond(this);
	}
	
	return rec_size;
}

packet next_packet(Slider* this, FILE* stream){
	packet msg;
	msg.error = true;

	msg.type = data;
	msg.seq = this->seq;
	this->seq = mod(this->seq +1);
	
	msg.size = fread(buf, 1, data_max, stream);
	if(msg.size > 0){
		msg.data_p = (uint8_t*)malloc(msg.size);
		memcpy(msg.data_p, buf, msg.size);

		msg.parity = parity(msg);
	} else {
		
	}
	return msg;
}

/**
 * @brief fill window from to end, from always gets filled
 */
void fill_window(Slider* this, int from, FILE* stream){
	// 3 4 5 6	receives nack 5, given: start = 0; [0].seq == 3
	// 7 8 5 6	start = 2; from 0 to <start, new packets into window
	int i = from;
	do {
		this->window.arr[i] = next_packet(this, stream);
		this->window.arr[i].error = true;
		
		i = mod(i+1);
	} while (i != mod(q_end(this->window) +1));
}

int send_data(Slider* this, FILE* stream){
	int buf_i = 0;
	packet response;
	int result;
	int prev_start = this->window.start;
	
	
	while(ended){
		if(fill){
			fill_window(this, prev_start, stream);
		}
		
		fill = true;
		
		// send_appr()
		printf("Sending window\n");
		int i = this->window.start;
		do {
			if(this->window.arr[i].error){
				this->window.arr[i].error = false;
				print(this->window.arr[i])
				// send_msg(this->sock, this->window.arr[i]);
			}
			
			i = mod(i+1);
		} while(i != mod(q_end(this->window) +1));
		// end send_appr()
		
		
		printf("Receive reply? ");
		int reply;
		scanf("%d", reply);
		if(reply > 0){
			continue;
		}
		// with timeout
//		if(rec_packet(int sock, packet* msg_p, uint8_t* buf) == FAIL){
//			continue;
//		}
		printf("enter seq = ");
		scanf("%x", response.seq);
		printf("enter size = ");
		scanf("%x", response.size);
		response.data_p = (uint8_t*)malloc(response.size);
		for(int i = 0; i < response.size; i++){
			response.data_p[i] = i;
		}
		response.parity = parity(response);
		print(response);
		
		// replied_window()
		int prev_start = this->window.start;
		
		int response_seq = response.data_p[0];
		switch(response.type){
			case nack:
				at_seq(this->window, response_seq).error = true;
				// 3 4 5 6	receives nack 5, given: start = 0; [0].seq == 3
				// 7 8 5 6	start = 2; from 0 to <start, new packets into window
				this->window.start = seq_to_i(&this->window, response_seq);
				// 3 4 5 6	receives nack 3; start = 0; no new packets
				if(prev_start == this->window.start){
					fill = false;
				}
				break;
			case ack:
				
				// 3 4 5 6	receives ack 5, given: start = 0; [0].seq == 3
				// 7 8 9 6	start = 3; from 0 to <start, new packets into window
				// 7 8 9 A	receives ack 6;	start = 0; from 0 to <0
				this->window.start = mod(seq_to_i(&this->window, response_seq) +1);
				break;
			default:
		}
		// end replied_window()
	}
	
	return buf_n;
}

#endif