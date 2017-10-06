
#define window_size 3

typedef struct {
	packet arr[window_size];
	int start, end, size;
	// index of the last msg ack
	int seq_i;
} PacketQueue;

void init(PacketQueue* this){
	this->start = 0;
	this->end = 0;
	this->size = 0;
	this->seq_i = window_size - 1;
}

packet front(PacketQueue* this){
	return this->arr[this->start];
}

int pop(PacketQueue* this){
	if(this->size > 0){
		this->start = (this->start + 1) % window_size;
		this->size -= 1;
		return this->size;
	} else {
		return FAIL;
	}
}

int push_back(PacketQueue* this, packet msg){
	if(size != window_size){
		this->arr[this->end] = msg;
		this->end = (this->end + 1) % window_size;
		this->size += 1;
		return this->size;
	} else {
		return FAIL;
	}
}

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
	
	send_ack(this->seq);
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