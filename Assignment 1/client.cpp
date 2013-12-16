/**
* This is the main for client.
* It creates a tcp connection. Negotiates with server for udp port.
* It creates a UDP socket and sends a string message.
* It receives a reversed string through the UDP port.
*/

#include "UDPSocket.h"
#include "TCPClientSocket.h"
#include <iostream>

int main(int argc, char *argv[]) {
    
    //parse args
    if (argc != 4) {
        std::cout << "This program requires 3 arguments in the following format!" << std::endl;
        std::cout << "client ipAddress portNumber \"Any string message here\"" << std::endl;
        std::cout << "Example: client 192.123.45.87 11234 \"Life is Beautiful\"" << std::endl;
        return 1;
    }

    const char* ipAddr = argv[1];
    if (0 == strcmp(ipAddr, "localhost")) {
    	ipAddr = "127.0.0.1";
    }

    int remotePort = atoi(argv[2]);
    const char* msg = argv[3];
    LOG("Client Message: %s\n", msg);

    //make a tcp connection and negotiate
	TCPClientSocket tcpClientSocket;

	unsigned long remoteIp = inet_addr(ipAddr);
	int ret = tcpClientSocket.initialize(remoteIp, remotePort);
	ASSERT(ret == 0);

	int serverPort = tcpClientSocket.negotiate();
	LOG("Received server UDP port: %i\n", serverPort);
	ASSERT(serverPort > 0);

	//create a udp socket
	UDPSocket udpSocket;
	udpSocket.open(CLIENT_DEFAULT_UDP_PORT);
	LOG("Client opened UDP port at %i\n", CLIENT_DEFAULT_UDP_PORT);

	//send a string and read reversed string from the socket
	int writeBytes = udpSocket.write(remoteIp, serverPort, msg, strlen(msg));
	ASSERT(writeBytes > 0);
	LOG("Client sent: %s\n", msg);

	char buf[1024];
	unsigned long ip; int port;
	int readBytes = udpSocket.read(buf, 1024, ip, port);
	buf[readBytes] = '\0';
	LOG("Client received: %s\n", buf);
	ASSERT(readBytes > 0);

	udpSocket.close();

	return 0;
}
