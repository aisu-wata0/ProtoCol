
#define window_size 3
#define mod(X) (X % window_size)

typedef struct {
	packet arr[window_size];
	// end index is by definition mod(start -1)
	int start;
	// index of the last msg without error, starting from 'start'
	int seq_i;
} PacketQueue;

void init(PacketQueue* this){
	this->start = 0;
	this->seq_i = window_size - 1;
	for(int i = this->start; i < mod(this->start -1);){
		this->arr[i].error = true;
		
		i = mod(i+1);
	}
}

packet front(PacketQueue* this){
	return this->arr[this->start];
}

packet last_ok(PacketQueue* this){
	return this->arr[this->seq_i];
}

int indexes_remain(PacketQueue* this){
	return mod((this->start -1) - this->seq_i);
}

//int pop(PacketQueue* this){
//	if(this->size > 0){
//		this->start = (this->start + 1) % window_size;
//		this->size -= 1;
//		return this->size;
//	} else {
//		return FAIL;
//	}
//}
//
//int push_back(PacketQueue* this, packet msg){
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
	
	PacketQueue queue;
	
	PacketQueue window;
	
	int seq;
} Slider;

void init(Slider* this, char* device){
	this->sock = raw_socket_connection(device);
	buf = (uint8_t*)malloc(BUF_MAX);
	
	init(queue);
	
	init(window);
	
	this->seq = 0;
}

/**
 * @brief if there's packets waiting and space in the queue, push packets
 */
void update_queue(Slider* this){
	int result = 1;
	packet msg;
	
	while(result > 0 && this->queue.size < window_size){
		result = try_packet(this->sock, &msg, this->buf);
		if(result > 0){
			if(msg.error){
				send_nack(msg.seq);
			} else if(msg.seq == this->seq){
				push_back(this->queue, msg);
			} else if(msg.seq > this->seq){
				// (msg.seq - back(this->queue).seq) == how much more space you need to store this message
				// if msg.seq comes after the back packet it how need n more indexes, where n is how ahead is the msg
				// eg: if the back elem is 2, you receive msg.seq = 4, you should need 2 more indexes
				// [2,  , 4], where 3 will be invalid, now 4 is the back elem
				// if the packet on the back has seq greater than msg.seq, it already should have space in the array
				if( (window_size - this->queue.size) >= (msg.seq - back(this->queue).seq) ){
					this->queue.arr[ (this->queue.seq_i + (msg.seq - this->seq)) % window_size ] = msg;
					this->queue.seq_i + 1 + (msg.seq - this->seq) % window_size
				}
			}
		}
	}
	
	send_ack(this->seq -1);
}

packet rec_packet(Slider* this){
	packet msg;
	int result;
	
	// block until at least 1 msg
	if(this->queue.size == 0){
		rec_packet(this->sock, &msg, this->buf);
		push_back(this->queue, msg);
	}
	
	msg = front(this);
	pop(this);
	update_queue(this);
	
	return msg;
}


int handle_msg(Slider* this, packet msg){
	/**
	 * [0 1 _ _ _ 5]
	 *  s l q       e
	 * 
	 * [8 3 4 _ _ _]
	 *   es l
	 * q = 5       
	 * arrive: msg.seq = 6
	 * 6-4 = 2
	 * l + 2 = 4
	 * [4] = msg
	 */
	
	if(msg.error){
		send_nack(this->sock, seq);
	} else if(msg.seq >= seq){
		// 1 if msg is the proper next in the sequece, 2 if skipped one message etc.
		int delta = (msg.seq - (seq-1));
		int msg_pos = mod(this->queue.seq_i + delta);
		// if hadn't received this msg yet
		if(this->queue.arr[msg_pos].error){
			this->queue.arr[msg_pos] = msg;
			// update which is the last message received without errors in the sequence
			while( (indexes_remain(this->queue) > 0) && (!this->queue.arr[mod(this->queue.seq_i + 1)].error) ){
			// [0 _ 2 3 4], seq_i == 0, receive 1
			// [0 1 2 3 4], seq_i == 4 == mod(start-1) (maximum)
				this->queue.seq_i = mod(this->queue.seq_i +1);
				seq += 1;
			}
		}
	}
	
	return this->queue.seq_i;
}



void ack_data(Slider* this){
	send_ack(last_ok(this->queue).seq);
	for(int i = this->queue.start; i < mod(this->queue.seq_i +1);){
		this->queue.arr[i].error = true;
		
		i = mod(i+1);
	}
	this->queue.start = mod(this->queue.seq_i +1);
}

void print_slider(Slider* this){
	for(int i = 0; i < window_size; i++){
		printf(" %x", i % 0xf);
	}
	for(int i = 0; i < window_size; i++){
		packet msg = this->queue.arr[i];
		if(msg.error){
			printf("  ");
		} else {
			printf(" %x", msg.seq % 0xf);
		}
	}
}

int receive_data(Slider* this, int data_size){
	uint8_t* buf; //to receive data from sock
	buf = (uint8_t*)malloc(data_size);
	int buf_n = 0;
	
	int result;
	
	while(buf_n != data_size){
		while( indexes_remain(this->queue) > 0 && (  this->queue.seq_i != (this->queue.end -1) )){
			// block until at least 1 msg
			// rec_packet(this->sock, &msg, this->buf);
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
			
			int i = mod(this->queue.seq_i + 1);
			handle_msg(this, msg);
			
			for(; i <= this->queue.seq_i;){
				memcpy(&buf[buf_n], this->queue.arr[i].data_p, msg.size);
				buf_n += this->queue.arr[i].size;
				
				int i = mod(i + 1);
			}
		}
		ack_data(this);
	}
	
	return buf_n;
}

int send_data(Slider* this, uint8_t* buf, int buf_n){
	int buf_i = 0;
	
	int result;
	
	while(front(this->window).type != end){
		while( indexes_remain(this->queue) > 0 && (  this->queue.seq_i != (this->queue.end -1) )){
			// block until at least 1 msg
			rec_packet(this->sock, &msg, this->buf);
			int i = mod(this->queue.seq_i + 1);
			handle_msg(this, msg);
			for(; i <= this->queue.seq_i;){
				
				memcpy(&buf[buf_n], this->queue.arr[i].data_p, msg.size);
				buf_n += this->queue.arr[i].size;
				
				int i = mod(i + 1);
			}
		}
		ack_data(this);
	}
	
	return buf_n;
}