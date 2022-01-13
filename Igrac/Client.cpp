#define WIN32_LEAN_AND_MEAN
#pragma warning( disable : 4996)

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "conio.h"
#include <time.h>
#include <stdint.h>
#include "Client.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#pragma pack(1)

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 27016
#define BUFFER_SIZE 256

int brojZaPogadjanje;

int prosek(int donja, int gornja) {
	return (donja + gornja) / 2;
}

int randomBr(int donja, int gornja) {
	srand((unsigned)time(NULL));
	return (rand() % (gornja - donja + 1) + donja);
}

int traziSeBr(int donja, int gornja) {
	srand((unsigned)time(NULL));
	return (rand() % (gornja - donja + 1) + donja);
}

void SendFunction(SOCKET connSocket, char dataBuffer[]);
void RecvFunction(SOCKET connSocket, char dataBuffer[]);

DWORD WINAPI GlavniThread(LPVOID lpParam)
{
	IGRACI* igraci = (IGRACI*)lpParam;
	char dataBuffer[BUFFER_SIZE];
	int iResult;
	int primljenBr = -1;
	bool tacan = false;

	SOCKET acceptedSocket = *(igraci->accSocket);

	unsigned long  mode = 1;
	if (ioctlsocket(acceptedSocket, FIONBIO, &mode) != 0)
		printf("ioctlsocket failed with error.");
	printf("Broj za pogadjanje je: %d \n", brojZaPogadjanje);

	while (!tacan)
	{
		while (1)
		{
			///Primi broj koji je potrebno proveriti
			iResult = recv(acceptedSocket, dataBuffer, BUFFER_SIZE, 0);    //ovaj recv ovde ne radi

			if (iResult > 0)
			{
				dataBuffer[iResult] = '\0';
				primljenBr = atoi(dataBuffer);
				break;
			}
			else if (iResult == 0)	// Check if shutdown command is received
			{
				// Connection was closed successfully
				printf("Connection with server closed.\n");
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

		if (primljenBr == brojZaPogadjanje)
		{
			strcpy(dataBuffer, "TACNO");
			tacan = true;
		}
		else if (primljenBr < brojZaPogadjanje)
		{
			strcpy(dataBuffer, "VECE");
		}
		else
		{
			strcpy(dataBuffer, "MANJE");
		}

		///Saljem moj odgovor
		// Send message to server using connected socket
		iResult = send(acceptedSocket, dataBuffer, (int)strlen(dataBuffer), 0);

		// Check result of send function
		if (iResult == SOCKET_ERROR)
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSocket);
			WSACleanup();
			return 1;
		}
	}

	printf("\n************************************************************\n");
	printf("\nKRAJ IGRE!!!\n");
	_getch();
	return 0;
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
		printf("%s.\n", dataBuffer);

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

// TCP client that use non-blocking sockets
int main()
{
	// Socket used to communicate with server
	SOCKET connectSocket = INVALID_SOCKET;

	// Variable used to store function return value
	int iResult;

	// Buffer we will use to store message
	char dataBuffer[BUFFER_SIZE];

	// WSADATA data structure that is to receive details of the Windows Sockets implementation
	WSADATA wsaData;

	// Initialize windows sockets library for this process
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return 1;
	}

	// create a socket
	connectSocket = socket(AF_INET,
		SOCK_STREAM,
		IPPROTO_TCP);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	// Create and initialize address structure
	sockaddr_in serverAddresstemp;
	serverAddresstemp.sin_family = AF_INET;								// IPv4 protocol
	serverAddresstemp.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);	// ip address of server
	serverAddresstemp.sin_port = htons(SERVER_PORT);					// server port

	// Connect to server specified in serverAddress and socket connectSocket
	iResult = connect(connectSocket, (SOCKADDR*)&serverAddresstemp, sizeof(serverAddresstemp));
	if (iResult == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	int ind;
	int32_t ret;
	char* podatak = (char*)&ret;
	brojZaPogadjanje = -1;

	IGRACI glavniThread;
	DWORD glavniThreadID;
	HANDLE handleGlavni = NULL;

	iResult = recv(connectSocket, podatak, sizeof(ret), 0);
	if (iResult > 0)
	{
		ind = ntohl(ret);
	}

	//RecvFunction(connectSocket, dataBuffer);
	/*
	memset(dataBuffer, 0, BUFFER_SIZE);
	sprintf(dataBuffer, "pocetniigrac");
	SendFunction(connectSocket, dataBuffer);*/

	///Slanje intervala od strane igraca koji zamislja broj
	printf("Unesite interval u kom se zamisljeni broj nalazi u formatu DONJI-GORNJI\n");
	scanf("%s", &dataBuffer);

	// Send message to server using connected socket
	SendFunction(connectSocket, dataBuffer);

	char* odvoji = strtok(dataBuffer, "-");
	short donji = atoi(odvoji);
	odvoji = strtok(NULL, "-");
	short gornji = atoi(odvoji);
	brojZaPogadjanje = traziSeBr(donji, gornji);

	glavniThread.interval.donji = donji;
	glavniThread.interval.gornji = gornji;
	glavniThread.accSocket = &connectSocket;
	glavniThread.serverAddress = serverAddresstemp;
	handleGlavni = CreateThread(NULL, 0, &GlavniThread, &glavniThread, 0, &glavniThreadID);

	// Shutdown the connection since we're done
	Sleep(1000000);

	CloseHandle(handleGlavni);
	
	iResult = shutdown(connectSocket, SD_BOTH);

	// Check if connection is succesfully shut down.
	if (iResult == SOCKET_ERROR)
	{
		printf("Shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	// Close connected socket
	closesocket(connectSocket);

	// Deinitialize WSA library
	WSACleanup();

	return 0;
}