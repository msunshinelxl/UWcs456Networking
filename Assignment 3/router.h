#include <map>
#include <list>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <queue>
#include <fstream>

using namespace std;

#define TOTAL_ROUTERS 5

#define MAX_COST 0x0fffffff  //easier to not deal with unsigned ints
#define UNKNOWN_ROUTER 0

typedef unsigned int U32;

struct pkt_HELLO {
	U32 router_id;
	U32 link_id;
};

struct pkt_LSPDU {
	U32 sender;
	U32 router_id;
	U32 link_id;
	U32 cost;
	U32 via;
};

struct pkt_INIT {
	U32 router_id;
};

struct link_cost {
	U32 link;
	U32 cost;
};

struct circuit_DB {
	U32 num_links;
	struct link_cost linkcost[TOTAL_ROUTERS];
};

U32 g_routerId;
unsigned long g_remoteIp;
U32 g_remotePort;
UDPSocket g_socket;

ofstream file;

/* 
The actual routing table with cost.
Update them at the same time
RIB - routing information base
*/
map<U32, U32> g_rib_path; // <destination router, next-hop-router>
map<U32, U32> g_rib_cost; // <destination router, cost>

map<U32, U32> g_link_cost; // <link, cost>	

/*
Map neighbor routers to link.
Should be updated only once when hello packets are received
*/
map<U32, U32> g_neighbors;	// <neighbor router, link>

/*
Topology Database
*/
list<U32>* g_router_topology[TOTAL_ROUTERS]; // links for each router, get costs from g_link_cost

// <link, list of routers in the link> inverse of g_router_topology, the two routers for each link
map<U32, list<U32>* > g_link_topology;


struct LINK {
    U32 router_id;
    U32 link_id;
    U32 cost;
};

struct compare {
	bool operator()(LINK const & l, LINK const & r) {
		return (l.cost > r.cost);
	}
};

void initialize() {
    for (U32 i=0; i<TOTAL_ROUTERS; ++i) {
        g_router_topology[i] = new list<U32>();

        g_rib_path.insert(pair<U32, U32>(i+1, UNKNOWN_ROUTER));
        g_rib_cost.insert(pair<U32, U32>(i+1, MAX_COST));
    }
}

void print_topology() {
	file << "# Topology Database\n";
	for (U32 i=0; i < TOTAL_ROUTERS; ++i) {
		file << "R" << g_routerId << " -> " << "R" << i+1 << " Num Links " << g_router_topology[i]->size() << "\n";
		for (list<U32>::iterator itr = g_router_topology[i]->begin(); itr != g_router_topology[i]->end(); ++itr) {
			file << "R" << g_routerId << " -> " << "R" << i+1 << " link " << *itr << " cost " << g_link_cost.find(*itr)->second << "\n";
		}
	}
	file.flush();
}

void print_rib_info() {
    LOG("RIB INFO \n");
    file << "#RIB\n";
    LOG("Router,    Next Hop,   Cost\n");
    for (U32 i=1; i<TOTAL_ROUTERS+1; ++i) {
        U32 next_hop = g_rib_path.find(i)->second;

        if (next_hop == 0) {
            continue;
        }

        U32 cost = g_rib_cost.find(i)->second;

        if (g_routerId == i) {
            LOG("Router: %i, next hop: local, cost: %i\n", i, cost);
            file << "R" << g_routerId << " -> " << "R" << i << " -> Local, 0\n";
        } else {
            LOG("Router: %i, next hop: %i, link: %i, cost: %i\n", i, next_hop, g_neighbors.find(next_hop)->second, cost);
            file << "R" << g_routerId << " -> " << "R" << i << " -> R" << next_hop << ", " << cost << "\n";
        }        
    }
    LOG("\n");
    file.flush();
}

void sendInitPacket() {
    Packet *initPacket = Packet::createINIT(g_routerId);
    const char* udpData = initPacket->getUDPData();
    g_socket.write(g_remoteIp, g_remotePort, udpData, initPacket->getLength());
    free((void *) udpData);
    delete(initPacket);
    LOG("Sent INIT Pakcet \n");
    file << "R" << g_routerId << " --> INIT packet sent\n";
    file.flush();
}   

void sendHelloPacket(U32 link_id) {
    Packet* helloPacket = Packet::createHELLO(g_routerId, link_id);
    const char *udpData = helloPacket->getUDPData();
    g_socket.write(g_remoteIp, g_remotePort, udpData, helloPacket->getLength());
    free((void *) udpData);
    delete(helloPacket);
    LOG("Sent HELLO through link %i \n", link_id);
    file << "R" << g_routerId << " --> HELLO packet sent on link " << link_id << "\n";
    file.flush();
}

void sendLSPDU(U32 router_id, U32 link_id, U32 cost, U32 via ) {
    Packet* pduPacket = Packet::createLSPDU(g_routerId, router_id, link_id, cost, via);
    const char* udpData = pduPacket->getUDPData();
    g_socket.write(g_remoteIp, g_remotePort, udpData, pduPacket->getLength());
    free((void *) udpData);
    delete(pduPacket);
    LOG("Sent PDU Packet via %i <-- DATA: router %i, link %i, cost %i \n", via, router_id, link_id, cost);
    file << "R" << g_routerId << " sent LS PDU: " << ", router_id " << router_id << 
            ", link_id " << link_id << ", cost " << cost << ", via" << via << "\n";
    file.flush();
}

void sendDBToNeighbor(U32 routerId, struct circuit_DB* circuit_db) {
    //Send Circuit DB
    for (U32 i=0; i< circuit_db->num_links; ++i) {
        U32 link = circuit_db->linkcost[i].link;
        U32 cost = circuit_db->linkcost[i].cost;
        U32 via = g_neighbors.find(routerId)->second;
        sendLSPDU(g_routerId, link, cost, via);
    }
}