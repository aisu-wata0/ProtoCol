
#define window_size 3

typedef struct {
	int sock;
	uint8_t* buf; //to receive data from sock
	
	packet window[window_size+1];
	int start, end;
	int seq;
} Slider;

void SliderC(Slider* this, char* device){
	this->sock = raw_socket_connection(device);
	buf = (uint8_t*)malloc(BUF_MAX);
	
	this->start = 0;
	this->end = 0;
	this->seq = 0;
}

packet rec_packet(Slider* this){
	packet msg;
	int result;
	result = rec_packet(this->sock, &msg, this->buf);
	queue.push()
	if(msg.error){
		send_nack(this->sock, msg);
	}
	
	return msg;
}