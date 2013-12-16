#include "UDPSocket.h"

//local port on which to open the socket
int UDPSocket::open(int localPort) {
	mLocalPort = localPort;
    
	memset(&mRemoteAddr, 0, sizeof(sockaddr_in));
	mRemoteAddr.sin_family = AF_INET;
	mRemoteAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	mRemoteAddr.sin_port = htons(mLocalPort);

	mSocket = socket(AF_INET, SOCK_DGRAM, 0);
	ASSERT(mSocket > 0);
	
	int ret = bind(mSocket, (sockaddr *) &mRemoteAddr, sizeof(sockaddr_in));
	ASSERT(ret == 0);
	
	return ret;
}

int UDPSocket::hasData() {
    int count = 0;
    int ret = ioctl(mSocket, FIONREAD, &count);
    ASSERT(ret == 0);
    return count;
}

//Reads into the buffer buf. The remoteIp and remotePort are outputs as well.
//The identify the source's process over the network.
int UDPSocket::read(char *buf, int size, unsigned long & remoteIp, unsigned int & remotePort) {
    sockaddr_in remoteAddr;
    int remoteAddrSize = sizeof(sockaddr_in);
    //LOG("Waiting on recv.. \n");
    int length = recvfrom(mSocket, buf, size, 0, (sockaddr *) &remoteAddr, 
                                                (socklen_t *) &remoteAddrSize);

    remoteIp = remoteAddr.sin_addr.s_addr;
    remotePort = ntohs(remoteAddr.sin_port);
	
    return length;                                                
}

//Writes the data into the network. The remotePort and remoteIp are inputs and they determine 
//the destination process. 
int UDPSocket::write(const unsigned long remoteIp, const unsigned int remotePort,  const char * const buf, const int size) {
    sockaddr_in remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_addr.s_addr = remoteIp;
    remoteAddr.sin_port = htons(remotePort);
    int length = sendto(mSocket, buf, size, 0, (sockaddr *) &remoteAddr, sizeof(remoteAddr));

    return length;
}

//close the udp socket
int UDPSocket::close() {
    return ::close(mSocket);
}
