/**
* A class that defines a TCP Client side socket.
* It is responsible for negotiation and establishing a tcp connection.
*/

#ifndef TCP_CLIENT_SOCKET_H
#define TCP_CLIENT_SOCKET_H

#include "Socket.h"

class TCPClientSocket {
private:
	SOCKET	    mSocket;
	sockaddr_in mRemoteAddr;

public:
    int initialize(unsigned long remoteIp, int remotePort);
    int negotiate();
};

#endif //TCP_CLIENT_SOCKET_H
