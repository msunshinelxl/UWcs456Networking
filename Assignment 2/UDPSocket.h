/**
* Udp socket is connectionless transport layer. 
* This socket API has been designed to be similar to the file I/O API.
* Note this class can be served as UdpSink or UdpSource.
*/

#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include "Socket.h"
#include <sys/ioctl.h>

class UDPSocket {
private:
	SOCKET mSocket;
    unsigned int mLocalPort;
    sockaddr_in  mRemoteAddr;
    
public:
	int open(int localPort);
	int hasData();
	int read(char * buf, int size, unsigned long & remoteIp, int & remotePort);
	int write(const unsigned long remoteIp, const unsigned int remotePort, const char * const buf, const int size);
	int close();
};

#endif //UDPSOCKET_H
