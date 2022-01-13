#pragma once
#include <WinSock2.h>
#include <winsock.h>
typedef struct pocetni_igrac {
	SOCKET acceptedSocket;
	char gornjiInterval[128];
	char donjInterval[128];
	char username[20];
}POCETNI_IGRAC;