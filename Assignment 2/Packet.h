// common packet class used by both SENDER and RECEIVER

#include "Socket.h"

using namespace std;

class Packet {
private:
	static const int UDP_BUFFER_SIZE = 512;
	static const int PACKET_HEADER_SIZE = 12;

public:
	static const unsigned int MAX_DATA_LENGTH = 500;
	static const unsigned int MAX_SEQ_NUMBER = 32;
	static const unsigned int MAX_PACKET_LENGTH = MAX_DATA_LENGTH + PACKET_HEADER_SIZE;
	
private:	
	// data members
	unsigned int type;
	unsigned int seqnum;
	unsigned int length;
	char data[MAX_DATA_LENGTH];
	
	//////////////////////// CONSTRUCTORS //////////////////////////////////////////
	
	// hidden constructor to prevent creation of invalid packets
	Packet(int Type, int SeqNum, int Length, const char* Data) {	
		type = Type;
		seqnum = SeqNum;
		length = Length;
		memcpy(data, Data, length);
	}

public:
	
	// special packet constructors to be used in place of hidden constructor
	static Packet* createACK(int SeqNum) {
		return new Packet(0, SeqNum, 0, "");
	}
	
	static Packet* createPacket(int SeqNum, int Length, const char * Buf) {
		return new Packet(1, SeqNum, Length, Buf);
	}
	
	static Packet* createEOT(int SeqNum) {
		return new Packet(2, SeqNum, 0, "");
	}
	
	///////////////////////// PACKET DATA //////////////////////////////////////////
	
	int getType() {
		return type;
	}
	
	int getSeqNum() {
		return seqnum;
	}
	
	int getLength() {
		return length;
	}
	
	char* getData() {
		return data;
	}
	
	//////////////////////////// UDP HELPERS ///////////////////////////////////////
	
	char* getUDPData() {
		char * buffer = (char *) malloc(sizeof(char) * UDP_BUFFER_SIZE);
		
		int Type = htonl(type);
		int SeqNum = htonl(seqnum);
		int Length = htonl(length);

		memcpy(buffer, &Type, sizeof(int));
		memcpy(buffer + sizeof(int), &SeqNum, sizeof(int));
		memcpy(buffer + 2*sizeof(int), &Length, sizeof(int));
		
		memcpy(buffer + 3*sizeof(int), data, length);
		return buffer;
	}

	int getUDPSize() {
		return (length + PACKET_HEADER_SIZE);
	}
	
	static Packet* parseUDPData(char * buffer) {
		int Type, Length, SeqNum;
		memcpy(&Type, buffer, sizeof(int));
		memcpy(&SeqNum, buffer + sizeof(int), sizeof(int));
		memcpy(&Length, buffer + 2*sizeof(int), sizeof(int));

		return new Packet(ntohl(Type), ntohl(SeqNum), ntohl(Length), buffer + 3*sizeof(int));
	}
};

/**
Packet Test Code

Packet* packet = Packet::createPacket(23, "abcedeweserfdasfadsfwe");

Packet* parsePacket = Packet::parseUDPData(packet->getUDPData());
ASSERT(parsePacket->getType() == 1);
ASSERT(parsePacket->getSeqNum() == 23);
ASSERT(parsePacket->getLength() == strlen("abcedeweserfdasfadsfwe"));
ASSERT(strcmp("abcedeweserfdasfadsfwe", parsePacket->getData()) == 0);

LOG("Tests pass!!\n");
*/