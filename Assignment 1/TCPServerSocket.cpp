#include "TCPServerSocket.h"

//contact servera and establish a tcp connection
int TCPServerSocket::initialize() {
    mLocalPort = SERVER_DEFAULT_TCP_PORT;

	memset(&mLocalAddr, 0, sizeof(sockaddr_in));
	mLocalAddr.sin_family = AF_INET;
	mLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	mLocalAddr.sin_port = htons(mLocalPort);
	
	mSocket = socket(AF_INET, SOCK_STREAM, 0);
	ASSERT(mSocket > 0);

    int ret = bind(mSocket, (struct sockaddr *) &mLocalAddr, sizeof(mLocalAddr));
	ASSERT(ret == 0);
	ret = listen(mSocket, 1);
	ASSERT(ret == 0);
	
	LOG("Successfully initialized server on port %i \n", mLocalPort);
	return 0;
}

//Negotiate with client and send the server's udp port
int TCPServerSocket::negotiate() {	
	
	unsigned char buf[1024];

	sockaddr_in clientAddr;
	socklen_t length;
	SOCKET connectionSocket = accept(mSocket, (sockaddr *) &clientAddr, &length);
	read(connectionSocket, buf, sizeof(buf));
	
	//dont really care about client udp port, it could be any random number to get client working
	int clientUdpPort;
	memcpy(&clientUdpPort, buf, sizeof(clientUdpPort));
	LOG("Server Received Request: %i\n", clientUdpPort);
	
	//send the default server port
	unsigned int serverUdpPort = SERVER_DEFAULT_UDP_PORT;
	memset(buf, 0, sizeof(buf));
	memcpy(buf, &serverUdpPort, sizeof(serverUdpPort));
	
	int writeBytes = write(connectionSocket, buf, sizeof(serverUdpPort));
	ASSERT(writeBytes > 0);
	
	//close the socket
	::close(connectionSocket);
	LOG("Server authorized the client for udp communications on port %i\n", serverUdpPort);

	return serverUdpPort;
}

void TCPServerSocket::close() {
	::close(mSocket);
}