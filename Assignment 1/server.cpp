/*
* This is the main file for a server. It waits for a TCP connection from client.
* It negotiates with client for a UDP port. It listens on udp port for a string message.
* It reverses the message and sends it back to the client.
*/

#include "TCPServerSocket.h"
#include "UDPSocket.h"
#include <algorithm>
#include <string>

void str_reverse(char* s) {
    int length = strlen(s) ;
    int i, j;
    char c;

	for (i = 0, j = length - 1; i < j; i++, j--)
	{
	    c = s[i];
	    s[i] = s[j];
	    s[j] = c;
	 }
}

int main() {
	//create tcp socket and establish a connection
    TCPServerSocket tcpServerSocket;

    int ret = tcpServerSocket.initialize();
    ASSERT(ret == 0);
    
    int serverUdpPort = tcpServerSocket.negotiate();
    ASSERT(serverUdpPort > 1024);

    tcpServerSocket.close();

    //open a udp socket and listen for str
    UDPSocket udpSocket;
    ret = udpSocket.open(serverUdpPort);
    ASSERT(ret == 0);
    LOG("Server opened UDP Port %i\n", serverUdpPort);

    char buf[1024];
    unsigned long remoteIp;
    int remotePort;
    
    //read, reverse string and send it back to client
	int readBytes = udpSocket.read(buf, 1024, remoteIp, remotePort);
	buf[readBytes] = '\0';
    ASSERT(readBytes > 0);
    LOG("Server Received: %s\n", buf);

    //Reverse the string and write to the buffer again
    str_reverse(buf);

    int writeBytes = udpSocket.write(remoteIp, remotePort, (const char *) buf, readBytes);
    ASSERT(writeBytes > 0);
    LOG("Server sent: %s\n", buf);

    udpSocket.close();

    return 0;
}
