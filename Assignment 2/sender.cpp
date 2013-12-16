/*
* This is the main file for sender. It creates a UDP socket and enables file transmission through it.
* First it reads the files and creates all the packets in a list.
* Next it starts transmitting the packets using the GO-BACK-N behaviour.
* The window size can be set in Socket.h
*/

#include "UDPSocket.h"
#include "Packet.h"
#include <fstream>
#include <list>
#include <sys/signal.h>
#include <iostream>

using namespace std;

/**
* Return -1 if a < b
* Return 0 if a == b
* Return 1 if a > b
*/
int comparePacket(unsigned int a, unsigned int b) {
    if (a <= Packet::MAX_SEQ_NUMBER && a >= (Packet::MAX_SEQ_NUMBER - WINDOW_SIZE) &&
        b >= 0 && b <= WINDOW_SIZE  ) {
        return -1;
    } else if (a >= 0 && a <= WINDOW_SIZE && 
               b <= Packet::MAX_SEQ_NUMBER && b >= (Packet::MAX_SEQ_NUMBER - WINDOW_SIZE) ) {
        return 1;
    } else if (a > b) {
        return 1;
    } else if (a < b) {
        return -1;
    } else if (a == b) {
        return 0;
    } else {
        //invalid case
        ASSERT(0);
    }

}

/**
* Sender can be in three states
*/
enum eSenderState {
    SENDER_SEND_DATA,
    SENDER_RECV_ACK,
    SENDER_RESEND_DATA
};

eSenderState g_state = SENDER_SEND_DATA;

bool g_timer_on = false;

/**
* Timer interrupt for no ack receipt.
*/
void signal_alarm_handler(int signal) {
    LOG("Inside INTERRUPT\n");
    g_state = SENDER_RESEND_DATA;
    g_timer_on = false;
}

const char* seqFile = "seqnum.log";
const char* ackFile = "ack.log";

int main(int argc, char* argv[]) {

    //parse args
    if (argc != 5) {
        std::cout << "This program requires 4 arguments in the following format!" << std::endl;
        std::cout << "sender <host address of emulator> <udp port number of emulator in FWD direction>" << "\n"
                  << "<udp port number of sender to receive acks> <fileName>" << std::endl;
        std::cout << "Example: sender 192.168.89.78 49998 11234 test.txt" << std::endl;
        return 1;
    }


    const char* ipAddr = argv[1];
    if (0 == strcmp(ipAddr, "localhost")) {
        ipAddr = "127.0.0.1";
    }

    unsigned long remoteIp = inet_addr(ipAddr);
    int remotePort = atoi(argv[2]);
    int senderPort = atoi(argv[3]);
    const char* fileName = argv[4];

    int totalPackets = 0;
    int totalBytes = 0;

    int sentPackets = 0;
    int sentBytes = 0;

    int seqNum = 0;
    signal(SIGALRM, signal_alarm_handler);
    list<Packet *> packetsToSend;
    list<Packet *> packetsSent;

    //Open the file and prepare the packets for transmission
    ifstream transferFile(fileName);
    ofstream seqLog(seqFile);
    ofstream ackLog(ackFile);

    if (!(seqLog.is_open() && ackLog.is_open())) {
        return 1;
    }

    if (transferFile.is_open()) {
        char buffer[Packet::MAX_DATA_LENGTH];

        while(false == transferFile.eof()) {
            transferFile.read(buffer, Packet::MAX_DATA_LENGTH);
            Packet* packet = Packet::createPacket(seqNum, transferFile.gcount(), buffer);
            packetsToSend.insert(packetsToSend.end(), packet);

            LOG("Inserted packet %i in the queue\n", totalPackets);

            seqNum++;
            seqNum %= Packet::MAX_SEQ_NUMBER;
            totalPackets++;
            totalBytes += packet->getLength();
        }
    } else {
        LOG("The input file was not found..\n");
        return 1;
    }

    transferFile.close();

    //Add EOT packet
    Packet *eotPacket = Packet::createEOT(seqNum);
    packetsToSend.insert(packetsToSend.end(), eotPacket);
    totalPackets++;

    LOG("Packets ready for transmission: %i\n", totalPackets);
    LOG("Total data to be sent: %i\n", totalBytes);

    //open a udp socket and listen for str
    UDPSocket udpSocket;
    int ret = udpSocket.open(senderPort);
    ASSERT(ret == 0);
    LOG("Sender is transmitting & receiving on UDP Port %i\n", senderPort);

    /**
    * Core Logic
    * The sender can be in three states
    * 1. SEND_DATA : Sends a packet as long as data is available and window size is permitted
    * 2. RECV_ACK : Receives an acknowledgement for a previously sent data
    * 3. RESEND_DATA: If an ACK is not received for a previously sent data within timer, resend the data
    */
    while ((packetsToSend.size() > 0) || (packetsSent.size() > 0)) {
        if (udpSocket.hasData()) {
            g_state = SENDER_RECV_ACK;
        }

        switch(g_state)
        {
            /**
            * Within the WINDOW_SIZE, if data is available, send it.
            * Remove it from packetsToSend, and add it to packetsSent
            * If timer is off, turn it on.
            */
            case SENDER_SEND_DATA: {
                if (packetsSent.size() < WINDOW_SIZE && packetsToSend.size() > 0) {
                    list<Packet *>::iterator itr = packetsToSend.begin();
                    Packet *packet = (*itr);
                    char *packetData = packet->getUDPData();
                    udpSocket.write(remoteIp, remotePort, packetData, packet->getUDPSize());
                    sentPackets++;
                    sentBytes += packet->getLength();
                    
                    seqLog << packet->getSeqNum() << endl;
                    LOG("Sent packet seqnum %i, packet number %i\n", packet->getSeqNum(), sentPackets);
                    LOG("Sent bytes: %i\n", sentBytes);
                    
                    if (false == g_timer_on) {
                        alarm(TIMER_SECS);
                        g_timer_on = true;
                        LOG("SEND_DATA, TIMER ON\n");
                    }

                    free(packetData);

                    packetsToSend.remove(packet);
                    packetsSent.insert(packetsSent.end(), packet);
                }
            } break;

            /**
            * 1. RECV Packet.
            * 2. Check if the ack number belongs to any packets in packetsSent.
            * 3. If yes, delete all packets upto and before.
            * 4. If not, discard packet
            * 5. If any packets are sent and not recieved ack, start timer again.
            */
            case SENDER_RECV_ACK: {
                char buffer[Packet::MAX_PACKET_LENGTH];
                unsigned long ip;
                int port; 
                int length = Packet::MAX_PACKET_LENGTH;
                udpSocket.read(buffer, length, ip, port);

                Packet *packet = Packet::parseUDPData(buffer);
                int seqNum = packet->getSeqNum();
                LOG("RECVED ACK, %i \n", seqNum);
                ackLog << seqNum << endl;
                
                //since it is cumulative keep deleting until seq num is found on packetsSent
                list<Packet *>::iterator itr = packetsSent.begin();
                while (itr != packetsSent.end() && comparePacket((*itr)->getSeqNum(), seqNum) <= 0) { //less than or equal
                    Packet *sentPacket = (*itr);
                    packetsSent.remove(sentPacket);
                    itr = packetsSent.begin();

                    delete(sentPacket);
                }

                delete(packet);

                if ((g_timer_on == false) && (packetsSent.size() > 0)) {
                    alarm(TIMER_SECS);
                    g_timer_on = true;
                    LOG("RECV_DATA, TIMER ON\n");
                }

                g_state = SENDER_SEND_DATA;

            } break;

            /**
            * 1. Resend data if timeout.
            * 2. Send all packets in packetsToSend
            */
            case SENDER_RESEND_DATA: {
                for (list<Packet *>::iterator itr = packetsSent.begin(); itr != packetsSent.end(); ++itr) {
                    Packet *packet = (*itr);
                    char *packetData = packet->getUDPData();
                    udpSocket.write(remoteIp, remotePort, packetData, packet->getUDPSize());

                    seqLog << packet->getSeqNum() << endl;
                    LOG("Resent packet seqnum %i\n", packet->getSeqNum());

                    free(packetData);
                }

                if (false == g_timer_on && packetsSent.size() > 0) {
                    alarm(TIMER_SECS);
                    g_timer_on = true;
                    LOG("RESEND_DATA, TIMER ON\n");
                }

                g_state = SENDER_SEND_DATA;

            } break;

            default: {
                ASSERT(0); //invalid state
            }
        }
    }

    LOG("End of Transmission. Exiting \n");
    udpSocket.close();
    seqLog.close();
    ackLog.close();

    ASSERT(totalBytes == sentBytes);
    ASSERT(totalPackets == sentPackets);
    
    return 0;
}
