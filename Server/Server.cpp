#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#define no_init_all

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "conio.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define SERVER_PORT 27016
#define BUFFER_SIZE 512
#define MAX_CLIENTS 3

typedef struct lista_igraca
{
	char username[20];
	int id;
	struct lista_igraca* sledeci;
}IGRACI;

//inicijalizovanje liste
IGRACI* prvi = NULL;
IGRACI* poslednji = NULL;

//funkcije
void RecvFunction(SOCKET accSocket, int i, char dataBuffer[]);
void SendFunction(SOCKET accSocket, char dataBuffer[]);
void DodajIgraca(char username[], int id);
void ispisiListu();


// TCP server that use blocking sockets
int main() 
{
    // Socket used for listening for new clients 
    SOCKET listenSocket = INVALID_SOCKET;

    // Socket used for communication with client
    SOCKET acceptedSocket[MAX_CLIENTS];

    // Variable used to store function return value
    int iResult;

    // Buffer used for storing incoming data
    char dataBuffer[BUFFER_SIZE];
    
	// WSADATA data structure that is to receive details of the Windows Sockets implementation
    WSADATA wsaData;

	// Initialize windows sockets library for this process
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return 1;
    }
   

	// Initialize serverAddress structure used by bind
	sockaddr_in serverAddress;
    memset((char*)&serverAddress,0,sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;				// IPv4 address family
    serverAddress.sin_addr.s_addr = INADDR_ANY;		// Use all available addresses
    serverAddress.sin_port = htons(SERVER_PORT);	// Use specific port

	memset(acceptedSocket, 0, sizeof(SOCKET));

    // Create a SOCKET for connecting to server
    listenSocket = socket(AF_INET,      // IPv4 address family
                          SOCK_STREAM,  // Stream socket
                          IPPROTO_TCP); // TCP protocol

	// Check if socket is successfully created
    if (listenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket - bind port number and local address to socket
	iResult = bind(listenSocket,(struct sockaddr*) &serverAddress,sizeof(serverAddress));

	// Check if socket is successfully binded to address and port from sockaddr_in structure
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

	unsigned long mode = 1;
	if (ioctlsocket(listenSocket, FIONBIO, &mode) != 0)
		printf("ioctlsocket failed with error: \n");

    // Set listenSocket in listening mode
    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

	fd_set readfds;
	timeval timeVal;
	timeVal.tv_sec = 0;
	timeVal.tv_usec = 0;
	
	int idPocetnog = -1;
	int lastIndex = 0;
	bool novaKonekcija = false;

    do
    {
		FD_ZERO(&readfds);

		if(lastIndex!=MAX_CLIENTS)
			FD_SET(listenSocket, &readfds);

		for(int i=0; i<lastIndex; i++)
			FD_SET(acceptedSocket[i], &readfds);

		int selectResult = select(0, &readfds, NULL, NULL, &timeVal);

		if (selectResult == SOCKET_ERROR)
		{
			printf("Select failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}
		else if (selectResult == 0)
		{
			continue;
		}
		else 
		{
			if (FD_ISSET(listenSocket, &readfds))
			{
				// Struct for information about connected client
				sockaddr_in clientAddr;

				int clientAddrSize = sizeof(struct sockaddr_in);

				// Accept new connections from clients 
				acceptedSocket[lastIndex] = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);

				// Check if accepted socket is valid 
				if (acceptedSocket[lastIndex] == INVALID_SOCKET)
				{
					printf("accept failed with error: %d\n", WSAGetLastError());
					closesocket(listenSocket);
					WSACleanup();
					return 1;
				}

				if (ioctlsocket(acceptedSocket[lastIndex], FIONBIO, &mode) != 0)
				{
					printf("ioctlsocket failed with error.");
					continue;
				}
				lastIndex++;

				novaKonekcija = true;

				memset(dataBuffer, 0, BUFFER_SIZE);
				sprintf_s(dataBuffer, "Unesite vas username (dozvoljena su samo mala slova): ");
				SendFunction(acceptedSocket[lastIndex - 1], dataBuffer);
			}
			for (int i = 0; i < lastIndex; i++)
			{
				if (FD_ISSET(acceptedSocket[i], &readfds))
				{
					if (novaKonekcija)
					{
						memset(dataBuffer, 0, BUFFER_SIZE);
						RecvFunction(acceptedSocket[i], i, dataBuffer);
						if (dataBuffer[strlen(dataBuffer) - 1] == 'D' && dataBuffer[strlen(dataBuffer) - 2] == 'A')
						{
							idPocetnog = i;
							printf("Pocetni igrac: %d", idPocetnog + 1);
						}

						DodajIgraca(dataBuffer, i);
						ispisiListu();

						if (i == idPocetnog)
						{
							//printf("ovdjeee saam");
							RecvFunction(acceptedSocket[i], i, dataBuffer);
						}
						novaKonekcija = false;
					}
					else
					{
						if (i == idPocetnog)
						{
							RecvFunction(acceptedSocket[i], i, dataBuffer);
						}
						else
						{
							RecvFunction(acceptedSocket[i], i, dataBuffer);
						}
					}
				}
			}
		}
    } while (true);

	closesocket(listenSocket);
	for (int i = 0; i < lastIndex; i++)
	{
		// Shutdown the connection since we're done
		iResult = shutdown(acceptedSocket[i], SD_BOTH);

		// Check if connection is succesfully shut down.
		if (iResult == SOCKET_ERROR)
		{
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSocket[i]);
			WSACleanup();
			return 1;
		}

		//Close listen and accepted sockets
		closesocket(acceptedSocket[i]);
	}

	// Deinitialize WSA library
    WSACleanup();

    return 0;
}
void RecvFunction(SOCKET accSocket, int i, char dataBuffer[])
{
	memset(dataBuffer, 0, BUFFER_SIZE);
	// Receive data until the client shuts down the connection
	int iResult = recv(accSocket, dataBuffer, BUFFER_SIZE, 0);

	if (iResult > 0)	// Check if message is successfully received
	{
		dataBuffer[iResult] = '\0';

		// Log message text
		printf("Client %d sent: %s.\n", i+1, dataBuffer);
		//strcpy_s(dataaBuffer, dataBuffer);
	}
	else if (iResult == 0)	// Check if shutdown command is received
	{
		// Connection was closed successfully
		printf("Connection with client closed.\n");
		closesocket(accSocket);

	}
	else	// There was an error during recv
	{

	}
}
void SendFunction(SOCKET accSocket, char dataBuffer[])
{
	// Send message to server using connected socket
	int iResult = send(accSocket, dataBuffer, (int)strlen(dataBuffer), 0);

	// Check result of send function
	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(accSocket);
		WSACleanup();
		//return 1;
	}
}
void DodajIgraca(char username[], int id)
{
	IGRACI *pomocni = (IGRACI*)malloc(sizeof(IGRACI));
	strcpy_s(pomocni->username, username);
	pomocni->id = id;

	if (prvi != NULL)
	{
		//printf("%d %s", pomocni->id, pomocni->username);
		poslednji->sledeci = pomocni;
		poslednji = pomocni;
		poslednji->sledeci = NULL;
		
	}
	else
	{
		//printf("%d %s", pomocni->id, pomocni->username);
		prvi = pomocni;
		poslednji = pomocni;
		prvi->sledeci = NULL;
		
	}
}
void ispisiListu()
{
	printf("\nISPIS LISTE: \n");
	IGRACI* pomocni = prvi;

	if (prvi != NULL)
	{
		while (pomocni != NULL)
		{
			printf(" %d", pomocni->id + 1);
			printf(" %s", pomocni->username);
			pomocni = pomocni->sledeci;
			printf("\n");
		}
	}
	else
	{
		printf("\nLista je prazna!");
	}
	printf("\n");
}