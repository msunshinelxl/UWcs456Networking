/**
* This is the main for client.
* It creates a tcp connection. Negotiates with server for udp port.
* It creates a UDP socket and sends a string message.
* It receives a reversed string through the UDP port.
*/

#include "UDPSocket.h"
#include "Packet.h"
#include <list>
#include <fstream>
#include <iostream>

using namespace std;

void sendAckPacket(UDPSocket & udpSocket, int seqNum, unsigned long remoteIp, unsigned int remotePort) {
	Packet *ackPacket = Packet::createACK(seqNum);
	char *udpData = ackPacket->getUDPData();
	int writeBytes = udpSocket.write(remoteIp, remotePort, udpData, ackPacket->getUDPSize());
	LOG("ACK %i sent \n", seqNum);
	ASSERT(writeBytes > 0);
	free(udpData);
	delete(ackPacket);
}

void sendEotPacket(UDPSocket & udpSocket, int seqNum, unsigned long remoteIp, unsigned int remotePort) {
	Packet *eotPacket = Packet::createEOT(seqNum);
	char *udpData = eotPacket->getUDPData();
	int writeBytes = udpSocket.write(remoteIp, remotePort, udpData, eotPacket->getUDPSize());
	ASSERT(writeBytes > 0);
	free(udpData);
	delete(eotPacket);
}

const char* arrivalFile = "arrival.log";

int main(int argc, char *argv[]) {
    
    //parse args
    if (argc != 5) {
        std::cout << "This program requires 4 arguments in the following format!" << std::endl;
        std::cout << "receiver <ipAddress of network emulator> <udp port of emulator to receive ACKs from receiver>" << endl
        << "<udp port number used by receiver to receive data> <fileName>" << std::endl;
        std::cout << "Example: receiver 192.123.45.87 49999 11235 recv_test.txt" << std::endl;
        return 1;
    }


    const char* ipAddr = argv[1];
    if (0 == strcmp(ipAddr, "localhost")) {
    	ipAddr = "127.0.0.1";
    }

    unsigned long remoteIp = inet_addr(ipAddr);
    int remotePort = atoi(argv[2]);
    int receiverPort = atoi(argv[3]);
    const char *recvFile = argv[4];

    list<Packet *> packetsReceived;
    unsigned int totalBytes = 0;
    unsigned int expectingSeqNum = 0;
    bool firstPacketReceived = false;

	//create a udp socket
	UDPSocket udpSocket;
	udpSocket.open(receiverPort);
	LOG("Receiver opened UDP port at %i\n", receiverPort);

	ofstream arrivalLog(arrivalFile);
	if (!arrivalLog.is_open()) {
		LOG("Arrival log file cannot be opened\n");
		return 1;
	}

	/**
	* RECV Packet.
	* If the sequence number is expected, add it to packets list. Send Ack.
	* If not discard and send previous ack.
	*/
	while (true) {
		char buf[Packet::MAX_PACKET_LENGTH];
		unsigned long ip; int port;
		udpSocket.read(buf, Packet::MAX_PACKET_LENGTH, ip, port);
		
		Packet *packet = Packet::parseUDPData(buf);
		arrivalLog << packet->getSeqNum() << endl;
		LOG("Received packet %i, length %i\n", packet->getSeqNum(), packet->getLength());

		if ((unsigned int) packet->getSeqNum() == expectingSeqNum) {
			if (packet->getType() == 1) { //data packet
				sendAckPacket(udpSocket, expectingSeqNum, remoteIp, remotePort);

				totalBytes += packet->getLength();
				LOG("Received %i bytes\n", totalBytes);

				expectingSeqNum++;
				expectingSeqNum %= Packet::MAX_SEQ_NUMBER;

				packetsReceived.insert(packetsReceived.end(), packet);

				LOG("Total packets received %i\n", (unsigned int) packetsReceived.size());

				if (false == firstPacketReceived) {
					firstPacketReceived = true;
				}
			} else if (packet->getType() == 2) { //eot packet
				LOG("EOT Packet received.. \n");
				sendEotPacket(udpSocket, expectingSeqNum, remoteIp, remotePort);
				break;
			}			
		} else {
			if (true == firstPacketReceived) {
				int previousSeqNum = expectingSeqNum - 1;
				previousSeqNum = (previousSeqNum + Packet::MAX_SEQ_NUMBER) % Packet::MAX_SEQ_NUMBER;

				sendAckPacket(udpSocket, previousSeqNum, remoteIp, remotePort);
			}

			delete(packet);
		}
	}

	udpSocket.close();

	LOG("Received %u packets \n", (unsigned int) packetsReceived.size());

	//Write to a file.
	ofstream transferFile(recvFile);
	if (transferFile.is_open()) {
		for (list<Packet *>::iterator itr = packetsReceived.begin(); itr != packetsReceived.end(); ++itr) {
			Packet *packet = (*itr);
			transferFile.write(packet->getData(), packet->getLength());
			delete(packet);
		}
	}
	LOG("Finished writing %u bytes to the file\n", totalBytes);

	transferFile.close();
	arrivalLog.close();

	return 0;
}
