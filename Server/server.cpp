#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>  /* Needed for getaddrinfo() and freeaddrinfo() */
#include <unistd.h>
#include <sys/types.h>
typedef int SOCKET;
typedef int INVALID_SOCKET;
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int sockInit(void)
{
#ifdef _WIN32
	WSADATA wsa_data;
	return WSAStartup(MAKEWORD(1, 1), &wsa_data);
#else
	return 0;
#endif
}

int sockQuit(void)
{
#ifdef _WIN32
	return WSACleanup();
#else
	return 0;
#endif
}

int Close(SOCKET &sock){
#ifdef _WIN32
	return closesocket(sock);
#else
	return close(sock);
#endif
}

int main(void)
{
	std::cout << "-------------------------------------------------------------------------------" << std::endl;
	std::cout << "\t[Server]\tWelcome to the Chatly" << std::endl;
	std::cout << "-------------------------------------------------------------------------------" << std::endl;
	int iResult;

	SOCKET ListenSocket = -1;
	SOCKET ClientSocket = -1;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN] = { 0 };
	int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = sockInit();
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	memset(&hints, 0, sizeof(hints)); // ß
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		sockQuit();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	u_long iMode = 1;
	//ioctlsocket(ListenSocket, FIONBIO, &iMode);
	if (ListenSocket == -1) {
		//printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		sockQuit();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = ::bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == -1) {
		//printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		Close(ListenSocket);
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == -1) {
		//printf("listen failed with error: %d\n", WSAGetLastError());
		Close(ListenSocket);
		return 1;
	}
	std::cout << "Waiting for the client to connect...";
	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == -1) {
		//printf("accept failed with error: %d\n", WSAGetLastError());
		Close(ListenSocket);
		return 1;
	}
	std::cout << "Done!" << std::endl;
	// No longer need server socket
	Close(ListenSocket); //ß
	//closesocket(ListenSocket);

	// Receive until the peer shuts down the connection
	do {
		char str[BUFSIZ] = { 0 };
		memset(recvbuf, 0, recvbuflen);
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);

		if (iResult > 0) {
			//printf("Bytes received: %d\n", iResult);
			std::cout << "[Client]: " << recvbuf << std::endl;
			// Echo the buffer back to the sender
			do{
				std::cout << "[Server]: ";

				std::cin.getline(str, sizeof(str));
				if (strlen(str) > 0)
					iSendResult = send(ClientSocket, str, (int)strlen(str), 0);
			} while (strlen(str) <= 0);
			if (iSendResult == -1) {
				//printf("send failed with error: %d\n", WSAGetLastError());
				Close(ListenSocket);
				return 1;
			}
			//printf("Bytes sent: %d\n", iSendResult);
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else  {
			//printf("recv failed with error: %d\n", WSAGetLastError());
			Close(ListenSocket);
			return 1;
		}

	} while (iResult > 0);

	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, 0x01);
	if (iResult == -1) {
		//printf("shutdown failed with error: %d\n", WSAGetLastError());
		Close(ListenSocket);
		sockQuit();
		return 1;
	}

	// cleanup
	Close(ListenSocket);
	sockQuit();
	return 0;
}
