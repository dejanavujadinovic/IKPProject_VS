#pragma once

typedef struct Interval {
	short gornji;
	short donji;
}INTERVAL;


typedef struct igracThread {
	SOCKET* accSocket;
	sockaddr_in serverAddress;
	INTERVAL interval;
	int* index;
	//char username[BUFFER_SIZE];
} SPOREDNI;
