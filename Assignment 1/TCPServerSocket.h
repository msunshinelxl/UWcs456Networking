/**
* TCP server socket handles the tcp connection on server side. 
* It negotiates with client and sends the UDP port on server.
*/

#ifndef TCP_SEVER_SOCKET_H
#define TCP_SERVER_SOCKET_H

#include "Socket.h"

class TCPServerSocket {
private:
	int    	    mLocalPort;
	SOCKET	    mSocket;
	sockaddr_in mLocalAddr;

public:
    int initialize();
    int negotiate();
    void close();
};

#endif //TCP_SERVER_SOCKET_H
