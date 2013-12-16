// common packet class used by both SENDER and RECEIVER

#include "Socket.h"

using namespace std;

class Packet {
public:
	static const int HELLO_PACKET_SIZE = 8;

private:
	static const int UDP_BUFFER_SIZE = 512;
	
private:	
	// data members
	unsigned int length;
	char data[UDP_BUFFER_SIZE];
	
	//////////////////////// CONSTRUCTORS //////////////////////////////////////////
	
	// hidden constructor to prevent creation of invalid packets
	Packet(const char* Data, int Length) {	
		length = Length;
		memcpy(data, Data, length);
	}

public:
	
	// special packet constructors to be used in place of hidden constructor
	static Packet* createINIT(int router_id) {
		return new Packet( (const char*) &router_id, sizeof(router_id));
	}

	static Packet* createHELLO(int router_id, int link_id) {
		int buffer[2];
		buffer[0] = router_id;
		buffer[1] = link_id;

		return new Packet((const char*) buffer, sizeof(buffer));
	}

	static Packet* createLSPDU(unsigned int sender, unsigned int router_id, 
							   unsigned int link_id, unsigned int cost, unsigned int via) {
		unsigned int buffer[] = { sender, router_id, link_id, cost, via };
		
		return new Packet((const char *) buffer, sizeof(buffer));
	}

		
	///////////////////////// PACKET DATA //////////////////////////////////////////
	
	int getLength() {
		return length;
	}
	
	char* getData() {
		return data;
	}
	
	//////////////////////////// UDP HELPERS ///////////////////////////////////////
	
	const char* getUDPData() {
		char * buffer = (char *) malloc(sizeof(char) * length);
		memcpy(buffer, data, length);
		return buffer;
	}
};