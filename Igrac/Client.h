#pragma once

typedef struct Interval {
	short gornji;
	short donji;
}INTERVAL;

typedef struct igraciThread {
	SOCKET* accSocket;
	sockaddr_in serverAddress;
	INTERVAL interval;
	int* index;
} IGRACI;