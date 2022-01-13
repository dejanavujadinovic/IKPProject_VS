#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "conio.h"
#include <time.h>
#include <stdint.h>
#include <string.h>
#include "Server.h"
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#pragma pack(1)

#define SERVER_PORT 27016
#define BUFFER_SIZE 256
#define MAX_CLIENTS 10

CVOR* prvi = NULL;


INTERVAL interval;
int trenutnoIgraca;
bool kraj;
HANDLE hSemafori[MAX_CLIENTS];
HANDLE hGlavniSemafor;
CRITICAL_SECTION cs;

CVOR* noviCvor(int* vrednost, int* ID, int* c)
{
	if (*c == 0)												
	{
		CVOR* novi = (CVOR*)malloc(sizeof(CVOR));
		novi->pogodak.vrednost = *vrednost;
		novi->pogodak.idKlijenta = *ID;
		novi->pogodak.proveren = false;
		novi->sledeci = prvi;
		prvi = novi;
		(*c)++;

		return novi;
	}
	else
	{
		CVOR* novi = (CVOR*)malloc(sizeof(CVOR));
		novi->pogodak.vrednost = *vrednost;
		novi->pogodak.idKlijenta = *ID;
		novi->pogodak.proveren = false;
		novi->sledeci = prvi;
		prvi = novi;
		(*c)++;

		return novi;
	}
}

void cvorIspisi()
{
	CVOR* cvor = prvi;
	printf("-----------------------------------\n");
	int i = 1;

	while (cvor != NULL)
	{
		printf("Vrednost: %d; ID klijenta: %d; Provereno: %d\n", cvor->pogodak.vrednost, cvor->pogodak.idKlijenta, cvor->pogodak.proveren);
		cvor = cvor->sledeci;
		i++;
	}
	printf("-----------------------------------\n");
}

void SendFunction(SOCKET connSocket, char dataBuffer[])
{
	// Send message to server using connected socket
	int iResult = send(connSocket, dataBuffer, (int)strlen(dataBuffer), 0);

	// Check result of send function
	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(connSocket);
		WSACleanup();
		//return 1;
	}

	printf("Message successfully sent. Total bytes: %ld\n", iResult);
}
void RecvFunction(SOCKET connSocket, char dataBuffer[])
{
	//char dataBuffer[BUFFER_SIZE];
	memset(dataBuffer, 0, BUFFER_SIZE);
	// Receive data until the client shuts down the connection
	int iResult = recv(connSocket, dataBuffer, BUFFER_SIZE, 0);

	if (iResult > 0)	// Check if message is successfully received
	{
		dataBuffer[iResult] = '\0';

		// Log message text
		//printf("%s.\n", dataBuffer);

	}
	else if (iResult == 0)	// Check if shutdown command is received
	{
		// Connection was closed successfully
		printf("Connection with client closed.\n");
		closesocket(connSocket);

	}
	else	// There was an error during recv
	{

	}
}

DWORD WINAPI GlavnaNit(LPVOID lpParam)
{
	IGRACI* igraci = (IGRACI*)lpParam;
	char dataBuffer[BUFFER_SIZE];
	int iResult;
	int* brCvorova = igraci->brojCvorova;
	int obavestenje;

	// Struct for information about connected client
	SOCKET acceptedSocket = *(igraci->accSocket);

	unsigned long  mode = 1;
	if (ioctlsocket(acceptedSocket, FIONBIO, &mode) != 0)
		printf("ioctlsocket failed with error.");
	printf("New client request accepted %d . Client address: %s : %d\n", *(igraci->ind), inet_ntoa(igraci->clientAddr.sin_addr), ntohs(igraci->clientAddr.sin_port));

	/*memset(dataBuffer, 0, BUFFER_SIZE);
	sprintf(dataBuffer, "Registrovanje...");
	SendFunction(acceptedSocket, dataBuffer);
	*/
	//RecvFunction(acceptedSocket, dataBuffer);
	//printf("%s", dataBuffer);

	while (1)
	{
		iResult = recv(acceptedSocket, dataBuffer, BUFFER_SIZE, 0);
		if (iResult > 0)	// Check if message is successfully received
		{
			dataBuffer[iResult] = '\0';
			break;
		}
		else if (iResult == 0)	// Check if shutdown command is received
		{
			// Connection was closed successfully
			printf("Connection with client closed.\n");
			closesocket(acceptedSocket);
		}
		else	// There was an error during recv
		{
			int error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK) // currently no data available
			{
				Sleep(50);  // wait and try again
				continue;
			}
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSocket);
		}
	}

	char* odvoji = strtok(dataBuffer, "-");
	interval.donji = atoi(odvoji);
	odvoji = strtok(NULL, "-");
	interval.gornji = atoi(odvoji);

	///Obavestavanje niti da salje klijentima interval
	ReleaseSemaphore(hSemafori[0], 1, NULL);

	///Cekanje da stigne prvi broj koji treba obraditi
	WaitForSingleObject(hGlavniSemafor, INFINITE);

	while (!kraj)
	{
		if (!(prvi->pogodak.proveren))
		{
			snprintf(dataBuffer, BUFFER_SIZE, "%d", prvi->pogodak.vrednost);

			///Slanje prvog broja klijentu koji je zamislio broj
			// Send message to client using connected socket
			iResult = send(acceptedSocket, dataBuffer, (int)strlen(dataBuffer), 0);

			// Check result of send function
			if (iResult == SOCKET_ERROR)
			{
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(acceptedSocket);
				WSACleanup();
				return 1;
			}
			printf("Proveravam broj: %d \n", prvi->pogodak.vrednost);

			while (1)
			{
				///Primanje odgovora o tom broju od klijenta koji je zamislio broj
				iResult = recv(acceptedSocket, dataBuffer, BUFFER_SIZE, 0);

				if (iResult > 0)	// Check if message is successfully received
				{
					dataBuffer[iResult] = '\0';
					printf("Glavni igrac je proverio, odgovor je: %s.\n", dataBuffer);

					EnterCriticalSection(&cs);

					strcpy(&(prvi->pogodak.odgovor), dataBuffer);
					if (!strcmp(&(prvi->pogodak.odgovor), "TACNO"))
						kraj = true;
					prvi->pogodak.proveren = true;
					obavestenje = prvi->pogodak.idKlijenta;

					LeaveCriticalSection(&cs);
					break;
				}
				else if (iResult == 0)	// Check if shutdown command is received
				{
					// Connection was closed successfully
					printf("Connection with client closed.\n");
					closesocket(acceptedSocket);
				}
				else	// There was an error during recv
				{
					int ierr = WSAGetLastError();
					if (ierr == WSAEWOULDBLOCK) // currently no data available
					{
						Sleep(50);  // wait and try again
						continue;
					}
					printf("recv failed with error: %d\n", WSAGetLastError());
					closesocket(acceptedSocket);
				}
			}

			ReleaseSemaphore(hSemafori[obavestenje], 1, NULL);
		}
		else
		{
			Sleep(50);
		}

	}

	printf("\n***************************************************************\n");
	printf("\nKRAJ IGRE!!!");
	_getch();

	return 0;
}

DWORD WINAPI SporednaNit(LPVOID lpParam)
{
	IGRACI* igraci = (IGRACI*)lpParam;
	int iResult;
	char dataBuffer[BUFFER_SIZE];
	int n = (int)(igraci->lpParam);
	int* brCvorova = igraci->brojCvorova;
	CVOR* cekamCvor;

	// Struct for information about connected client
	SOCKET acceptedSocket = *(igraci->accSocket);
	printf("New client request accepted %d . Client address: %s : %d\n", n + 1, inet_ntoa(igraci->clientAddr.sin_addr), ntohs(igraci->clientAddr.sin_port));


	/*memset(dataBuffer, 0, BUFFER_SIZE);
	sprintf(dataBuffer, "Registrovanje...");
	SendFunction(acceptedSocket, dataBuffer);
	*/
	/*RecvFunction(acceptedSocket, dataBuffer);
	printf("%s", dataBuffer);*/

	///Cekam da stigne interval koji mogu poslati svom klijentu
	WaitForSingleObject(hSemafori[n], INFINITE);

	char donji[BUFFER_SIZE];
	char gornji[BUFFER_SIZE];
	snprintf(gornji, BUFFER_SIZE, "%u", interval.gornji);
	snprintf(donji, BUFFER_SIZE, "%u", interval.donji);

	strcat(donji, "-");
	strcat(donji, gornji);

	strcpy(dataBuffer, donji);

	// Send message to client using connected socket
	///Saljem interval svom klijentu
	iResult = send(acceptedSocket, dataBuffer, (int)strlen(dataBuffer), 0);

	// Check result of send function
	if (iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(acceptedSocket);
		WSACleanup();
		return 1;
	}
	///Obavestavam sledeceg da posalje svom klijentu
	ReleaseSemaphore(hSemafori[(n + 1) % trenutnoIgraca], 1, NULL);

	while (1)
	{
		///Cekam da na mene dodje red da primim predlog svog klijenta
		WaitForSingleObject(hSemafori[n], INFINITE);

		while (1)
		{
			///Primam predlog mog klijenta
			iResult = recv(acceptedSocket, dataBuffer, BUFFER_SIZE, 0);

			if (iResult > 0)	// Check if message is successfully received
			{
				dataBuffer[iResult] = '\0';
				int vrednost = atoi(dataBuffer);

				EnterCriticalSection(&cs);

				cekamCvor = noviCvor(&vrednost, &n, brCvorova);

				LeaveCriticalSection(&cs);
				break;
			}
			else if (iResult == 0)	// Check if shutdown command is received
			{
				// Connection was closed successfully
				printf("Connection with client closed.\n");
				closesocket(acceptedSocket);
				return 1;
			}
			else	// There was an error during recv
			{
				int error = WSAGetLastError();
				if (error == WSAEWOULDBLOCK) // currently no data available
				{
					Sleep(50);  // wait and try again
					continue;
				}
				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(acceptedSocket);
			}
		}

		if (!kraj)
		{
			///Obavesti main da proveri za novi cvor koji sam dodao
			ReleaseSemaphore(hGlavniSemafor, 1, NULL);
		}
		else
		{
			strcpy(dataBuffer, "kraj");

			///Posalji poruku klijentu da je kraj igre
			// Send message to client using connected socket
			iResult = send(acceptedSocket, dataBuffer, BUFFER_SIZE, 0);

			// Check result of send function
			if (iResult == SOCKET_ERROR)
			{
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(acceptedSocket);
				return 1;
			}
			break;
		}

		///Cekam da moj zahtev bude obradjen
		WaitForSingleObject(hSemafori[n], INFINITE);

		strcpy(dataBuffer, &(cekamCvor->pogodak.odgovor));

		///Saljem odgovor svom igracu koji je poslao predlog
		// Send message to client using connected socket
		iResult = send(acceptedSocket, dataBuffer, BUFFER_SIZE, 0);

		// Check result of send function
		if (iResult == SOCKET_ERROR)
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSocket);
			WSACleanup();
			return 1;
		}
		///Obavesti sledecu nit da primi predlog svog klijenta
		ReleaseSemaphore(hSemafori[(n + 1) % trenutnoIgraca], 1, NULL);

		if (kraj)
		{
			break;
		}
	}
	return 0;
}

// TCP server that use non-blocking sockets
int main()
{
	// Socket used for listening for new clients 
	SOCKET listenSocket = INVALID_SOCKET;

	// Sockets used for communication with client
	SOCKET acceptedSockets[MAX_CLIENTS];
	

	// Variable used to store function return value
	int iResult;
	short lastIndex = 0;
	// WSADATA data structure that is to receive details of the Windows Sockets implementation
	WSADATA wsaData;

	HANDLE handleGlavni;
	HANDLE handleSporedni[MAX_CLIENTS];

	DWORD glavniThreadID;
	DWORD sporedniThreadID[MAX_CLIENTS];

	IGRACI glavniThread;
	IGRACI sporedniThread[MAX_CLIENTS];

	InitializeCriticalSection(&cs);

	interval.gornji = -1;
	interval.donji = -1;
	kraj = false;
	int brojCvorova = 0;

	// Initialize windows sockets library for this process
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return 1;
	}

	// Initialize serverAddress structure used by bind
	sockaddr_in serverAddress;
	memset((char*)&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;				// IPv4 address family
	serverAddress.sin_addr.s_addr = INADDR_ANY;		// Use all available addresses
	serverAddress.sin_port = htons(SERVER_PORT);	// Use specific port

	//initialise all client_socket[] to 0 so not checked
	memset(acceptedSockets, 0, MAX_CLIENTS * sizeof(SOCKET));

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
	iResult = bind(listenSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

	// Check if socket is successfully binded to address and port from sockaddr_in structure
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	unsigned long  mode = 1;
	if (ioctlsocket(listenSocket, FIONBIO, &mode) != 0)
		printf("ioctlsocket failed with error.");

	// Set listenSocket in listening mode
	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	printf("Server socket is set to listening mode. Waiting for new connection requests.\n");

	// set of socket descriptors
	fd_set readfds;

	// timeout for select function
	timeval timeVal;
	timeVal.tv_sec = 1;
	timeVal.tv_usec = 0;

	while (true)
	{
		// initialize socket set
		FD_ZERO(&readfds);

		// add server's socket and clients' sockets to set
		if (lastIndex != MAX_CLIENTS)
		{
			FD_SET(listenSocket, &readfds);
		}

		for (int i = 0; i < lastIndex; i++)
		{
			FD_SET(acceptedSockets[i], &readfds);
		}

		// wait for events on set
		int selectResult = select(0, &readfds, NULL, NULL, &timeVal);

		if (selectResult == SOCKET_ERROR)
		{
			printf("Select failed with error: %d\n", WSAGetLastError());
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}
		else if (selectResult == 0) // timeout expired
		{
			if (_kbhit()) //check if some key is pressed
			{
				Sleep(100);
				printf("Select char");
				_getch();
				printf("Timeout expired\n");
			}
			continue;
		}
		else if (FD_ISSET(listenSocket, &readfds))
		{
			// Struct for information about connected client
			sockaddr_in clientAddrtemp;
			int clientAddrSize = sizeof(struct sockaddr_in);

			// New connection request is received. Add new socket in array on first free position.
			acceptedSockets[lastIndex] = accept(listenSocket, (struct sockaddr*)&clientAddrtemp, &clientAddrSize);

			if (acceptedSockets[lastIndex] == INVALID_SOCKET)
			{
				if (WSAGetLastError() == WSAECONNRESET)
				{
					printf("accept failed, because timeout for client request has expired.\n");
				}
				else
				{
					printf("accept failed with error: %d\n", WSAGetLastError());
				}
			}
			else
			{
				if (ioctlsocket(acceptedSockets[lastIndex], FIONBIO, &mode) != 0)
				{
					printf("ioctlsocket failed with error.");
					continue;
				}
				lastIndex++;
				short id = lastIndex - 1;
				trenutnoIgraca = id;
				int32_t posaljiIndeks = htonl(id);
				char* data = (char*)&(posaljiIndeks);

				//Send index to client
				iResult = send(acceptedSockets[id], data, (int)sizeof(data), 0);

				// Check result of send function
				if (iResult == SOCKET_ERROR)
				{
					printf("send failed with error: %d\n", WSAGetLastError());
					closesocket(acceptedSockets[id]);
					WSACleanup();
					return 1;
				}

				if (id == 0)
				{
					glavniThread.accSocket = &acceptedSockets[id];
					glavniThread.ind = &(id);
					glavniThread.clientAddr = (clientAddrtemp);
					glavniThread.brojCvorova = &brojCvorova;
					hGlavniSemafor = CreateSemaphore(0, 0, 1, NULL);
					handleGlavni = CreateThread(NULL, 0, &GlavnaNit, &glavniThread, 0, &glavniThreadID);
					Sleep(500);
				}
				else
				{
					sporedniThread[id].accSocket = &acceptedSockets[id];
					sporedniThread[id].clientAddr = (clientAddrtemp);
					sporedniThread[id].lpParam = (LPVOID)(id - 1);
					sporedniThread[id].brojCvorova = &brojCvorova;
					hSemafori[id - 1] = CreateSemaphore(0, 0, 1, NULL);
					handleSporedni[id] = CreateThread(NULL, 0, &SporednaNit, &sporedniThread[id], 0, &sporedniThreadID[id]);
					Sleep(500);
				}
			}
		}

	}

	iResult = shutdown(listenSocket, SD_BOTH);

	// Check if connection is succesfully shut down.
	if (iResult == SOCKET_ERROR)
	{
		printf("Shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	CloseHandle(handleGlavni);
	for (int i = 1; i < trenutnoIgraca; i++)
	{
		CloseHandle(handleSporedni[i]);
	}
	DeleteCriticalSection(&cs);

	//Close listen and accepted sockets
	closesocket(listenSocket);
	for (int i = 0; i < trenutnoIgraca; i++)
	{
		closesocket(acceptedSockets[i]);
	}

	// Deinitialize WSA library
	WSACleanup();

	return 0;
}