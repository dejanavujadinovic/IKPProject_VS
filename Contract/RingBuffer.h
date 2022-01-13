#pragma once
#define RING_SIZE 256
// Kruzni bafer - FIFO 
struct RingBuffer {
	unsigned int tail;
	unsigned int head;
	unsigned char data[RING_SIZE];
};