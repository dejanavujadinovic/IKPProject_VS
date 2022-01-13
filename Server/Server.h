#pragma once

typedef struct Interval {
	short gornji;
	short donji;
}INTERVAL;

typedef struct pogodak {
	int vrednost;
	int idKlijenta;
	bool proveren;
	char odgovor;
}POGODAK;

typedef struct cvor {
	POGODAK pogodak;
	struct cvor* sledeci;
}CVOR;


typedef struct igraciThread {
	SOCKET* accSocket;
	short* ind;
	sockaddr_in clientAddr;
	LPVOID lpParam;
	int* brojCvorova;
} IGRACI;
