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

#define SERVER_IP_ADDRESS "127.0.0.1"
#define SERVER_PORT 27016
#define BUFFER_SIZE 512

//funkcije
void SendFunction(SOCKET connSocket, char dataBuffer[]);
void RecvFunction(SOCKET connSocket);

// TCP client that use blocking sockets
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
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
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
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;								// IPv4 protocol
	serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);	// ip address of server
    serverAddress.sin_port = htons(SERVER_PORT);					// server port

    // Connect to server specified in serverAddress and socket connectSocket
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
		return 1;
    }
	memset(dataBuffer, 0, BUFFER_SIZE);
	RecvFunction(connectSocket);

	do
	{
		// Read string from user into outgoing buffer
		//printf("Enter message to send: ");
		gets_s(dataBuffer, BUFFER_SIZE);
		SendFunction(connectSocket, dataBuffer);

		//memset(dataBuffer, 0, BUFFER_SIZE);
		//RecvFunction(connectSocket);
	
	} while (true);

	// Shutdown the connection since we're done
    iResult = shutdown(connectSocket, SD_BOTH);

	// Check if connection is succesfully shut down.
    if (iResult == SOCKET_ERROR)
    {
        printf("Shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

	// For demonstration purpose
	printf("\nPress any key to exit: ");
	_getch();


    // Close connected socket
    closesocket(connectSocket);

	// Deinitialize WSA library
    WSACleanup();

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
}
void RecvFunction(SOCKET connSocket)
{
	char dataBuffer[BUFFER_SIZE];
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