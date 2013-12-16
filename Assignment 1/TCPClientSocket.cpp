#include "TCPClientSocket.h"

/**
* Initialize a tcp connection. Create a socket and connect it with th remote address.
*/
int TCPClientSocket::initialize(unsigned long remoteIp, int remotePort) {
    
	memset(&mRemoteAddr, 0, sizeof(mRemoteAddr));
	mRemoteAddr.sin_family = AF_INET;
	mRemoteAddr.sin_addr.s_addr = remoteIp;
	mRemoteAddr.sin_port = htons(remotePort);
	
	mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ASSERT(mSocket > 0);

    int ret = connect(mSocket, (struct sockaddr *) &mRemoteAddr, sizeof(mRemoteAddr));
    ASSERT(ret == 0);
		
	return ret;
}

//Negotiate with the server and get the server's udp port.
int TCPClientSocket::negotiate() {	
	LOG("Negotiating with server \n");

	unsigned char buf[1024];
	unsigned int clientUdpPort = CLIENT_DEFAULT_UDP_PORT;
	memset(buf, 0, sizeof(buf));
	memcpy(buf, &clientUdpPort, sizeof(clientUdpPort));
	
	int writeBytes = write(mSocket, (void *) buf, sizeof(clientUdpPort));
	ASSERT(writeBytes > 0);

	unsigned int serverUdpPort;
	int readBytes = read(mSocket, buf, sizeof(buf));
	ASSERT(readBytes > 0);
	memcpy(&serverUdpPort, buf, sizeof(serverUdpPort));
	
	close(mSocket);
	return serverUdpPort;
}