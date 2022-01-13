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

int srednja_vrednost(int donja, int gornja) {
	return (donja + gornja) / 2;
}

int random_broj(int donja, int gornja) {
	srand((unsigned)time(NULL));
	return (rand() % (gornja - donja + 1) + donja);
}

int random_broj_koji_se_trazi(int donja, int gornja) {
	srand((unsigned)time(NULL));
	return (rand() % (gornja - donja + 1) + donja);
}

DWORD WINAPI SporedniNit(LPVOID lpParam)
{
	SPOREDNI* sporedni = (SPOREDNI*)lpParam;
	char dataBuffer[BUFFER_SIZE];
	int iResult;
	INTERVAL interval;
	int broj;

	SOCKET acceptedSocket = *(sporedni->accSocket);
	printf("Ja sam igrac ciji je broj: %d \n", *(sporedni->index));

	//RecvFunction(acceptedSocket, dataBuffer);

	/*memset(dataBuffer, 0, BUFFER_SIZE);
	sprintf(dataBuffer, "igrac %d", *(param->index));
	SendFunction(acceptedSocket, dataBuffer);*/

	unsigned long  mode = 1;
	if (ioctlsocket(acceptedSocket, FIONBIO, &mode) != 0)
		printf("ioctlsocket failed with error.");




	while (1)
	{
		///Primam inicijalni interval od servera
		iResult = recv(acceptedSocket, dataBuffer, BUFFER_SIZE, 0);

		if (iResult > 0)
		{
			dataBuffer[iResult] = '\0';
			printf("IGRA POCINJE!\n");
			char* odvoji = strtok(dataBuffer, "-");
			interval.donji = atoi(odvoji);
			odvoji = strtok(NULL, "-");
			interval.gornji = atoi(odvoji);
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

	printf("Broj je u intervalu: %d-%d\n", interval.donji, interval.gornji);

	while (1)
	{
		if (*(sporedni->index) == 1)
		{
			broj = srednja_vrednost(interval.donji, interval.gornji);
		}
		else
		{
			switch (*(sporedni->index) / 4)
			{
			case 1:
				broj = random_broj(interval.donji, interval.gornji) + 7;
				break;
			case 2:
				broj = random_broj(interval.donji, interval.gornji) - 7;
				break;
			case 3:
				broj = random_broj(interval.donji, interval.gornji) + 5;
				break;
			case 4:
				broj = random_broj(interval.donji, interval.gornji) - 5;
				break;
			default:
				broj = random_broj(interval.donji, interval.gornji) - 4;
				break;
			}
		}

		snprintf(dataBuffer, BUFFER_SIZE, "%d", broj);

		///Saljem moj pokusaj
		// Send message to server using connected socket
		SendFunction(acceptedSocket, dataBuffer);

		printf("Predlazem broj: %s\n", dataBuffer);

		while (1)
		{
			///Cekam odgovor za svoj predlog
			iResult = recv(acceptedSocket, dataBuffer, BUFFER_SIZE, 0);

			if (iResult > 0)
			{
				//dataBuffer2[iResult] = '\0';
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

		if (!strcmp(dataBuffer, "VECE"))
		{
			printf("Treba %s od predlozenog!\n", dataBuffer);
			interval.donji = broj;
		}
		else if (!strcmp(dataBuffer, "MANJE"))
		{
			printf("Treba %s od predlozenog!\n", dataBuffer);
			interval.gornji = broj;
		}
		else if (!strcmp(dataBuffer, "TACNO"))
		{
			printf("POGODAK!\n");
			break;
		}
		else
		{
			break;
		}
	}

	printf("\n**************************************************************\n");
	printf("\nKRAJ IGRE!!!\n");
	_getch();

	return 0;
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

	int index;
	int32_t ret;
	char* podatak = (char*)&ret;
	brojZaPogadjanje = -1;

	SPOREDNI threadParameter;
	DWORD ThreadSidePlayerID;
	HANDLE hSidePlayer = NULL;

	iResult = recv(connectSocket, podatak, sizeof(ret), 0);

	if (iResult > 0)
	{
		index = ntohl(ret);
	}
	


	printf("Uspesno ste se registrovali za pocetak igre. Molimo vas da sacekate, nova igra ce uskoro poceti...\n");
	threadParameter.accSocket = &connectSocket;
	threadParameter.serverAddress = serverAddresstemp;
	threadParameter.index = &index;
	hSidePlayer = CreateThread(NULL, 0, &SporedniNit, &threadParameter, 0, &ThreadSidePlayerID);

	// Shutdown the connection since we're done
	Sleep(1000000);

	CloseHandle(hSidePlayer);

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