/*
CS 456 - Assignment 3
Nagendra Boddula, 20344521
*/

#include "UDPSocket.h"
#include "Packet.h"
#include "router.h"
#include <sstream>

void handleHelloPacket(struct pkt_HELLO* helloPacket, struct circuit_DB* circuit_db) {
    U32 remoteRouter = helloPacket->router_id;
    U32 link = helloPacket->link_id;
    LOG("Received a hello packet from %i, link %i\n", remoteRouter, link);
    file << "R" << g_routerId << " -> HELLO packet from " << remoteRouter << " link " << link << "\n";
    file.flush();

    map<U32, U32>::iterator itr = g_neighbors.find(remoteRouter);
    if (g_neighbors.end() != itr ) {
        g_neighbors.erase(itr);
    }
    
    g_neighbors.insert(pair<U32, U32>(remoteRouter, link));
    
    //need to insert the link cost in the table as well
    for (U32 i=0; i< circuit_db->num_links; ++i) {
        if (circuit_db->linkcost[i].link == link) {
            g_link_cost.insert(pair<U32, U32>(link, circuit_db->linkcost[i].cost));
            break;
        }
    }

    /*now update topology
    Important: Update own routers topology.
    Other routers topology should be updated through LSPDUs
    */
    list<U32>* router_topology = g_router_topology[g_routerId - 1];
    router_topology->insert(router_topology->end(), link);

    map<U32, list<U32>* >::iterator l_iter = g_link_topology.find(link);
    if (g_link_topology.end() == l_iter) {
        g_link_topology.insert(pair<U32, list<U32>* >(link, new list<U32>()));
    }

    list<U32>* link_topology_router_list = g_link_topology.find(link)->second;
    if (link_topology_router_list->end() == find(link_topology_router_list->begin(), link_topology_router_list->end(), remoteRouter)) {        
        /*this is important, since this is initialization, the link topology needs to add own router to the other end of link
        it should not add the other end of the link as it would be added through PDUs
        */
        link_topology_router_list->insert(link_topology_router_list->end(), g_routerId);
    }
    ASSERT(link_topology_router_list->size() <= 2);
}

/*
The return variable tells if the packet changed topology.
If true, the RIB should be update
*/
bool handlePDUPacket(struct pkt_LSPDU* pduPacket) {
    U32 sender = pduPacket->sender;
    U32 router_id = pduPacket->router_id;
    U32 link = pduPacket->link_id;
    U32 cost = pduPacket->cost;
    U32 via = pduPacket->via;

    LOG("Received a LSPDU packet from %i --> DATA: router %i, link %i, cost %i, via %i\n", 
                                    sender,        router_id, link, cost, via); 

    file << "R" << g_routerId << " receives a LS PDU: sender " << sender << ", router_id " << router_id << 
            ", link_id " << link << ", cost " << cost << ", via" << via << "\n";
    file.flush();

    list<U32>* router_topology = g_router_topology[router_id - 1];

    //check if the link data was already available
    if (router_topology->end() == find(router_topology->begin(), router_topology->end(), link)) {
        LOG("Updating TOPOLOGY\n");
        router_topology->insert(router_topology->end(), link);
        
        if (g_link_cost.end() == g_link_cost.find(link)) {
            g_link_cost.insert(pair<U32, U32>(link, cost));    
        }

        map<U32, list<U32>* >::iterator l_iter = g_link_topology.find(link);
        if (g_link_topology.end() == l_iter) {
            g_link_topology.insert(pair<U32, list<U32>* >(link, new list<U32>()));
        }

        list<U32>* link_topology_router_list = g_link_topology.find(link)->second;
        if (link_topology_router_list->end() == find(link_topology_router_list->begin(), link_topology_router_list->end(), router_id)) {
            link_topology_router_list->insert(link_topology_router_list->end(), router_id);
        }
        ASSERT(link_topology_router_list->size() <= 2);

        //at this point the LSPDU must be forwarded to next router that is not the sender
        for (map<U32, U32>::iterator n_itr = g_neighbors.begin(); n_itr != g_neighbors.end(); ++n_itr) {
            if (sender != n_itr->first) {
                via = n_itr->second;

                sendLSPDU(router_id, link, cost, via);
            }
        }

        print_topology();

        return true;
    }
    return false;
}

/**
Update the router database and rib table 
1. construct a graph
2. Run Djikstra algorithm
3. Update RIB
*/
void updateRouterDatabase() {
    LOG("Updating Database \n");
    
    list<LINK> graph[TOTAL_ROUTERS + 1];

    //for contruct a graph
    for (map<U32, list<U32>* >::iterator l_iter = g_link_topology.begin(); l_iter != g_link_topology.end(); ++l_iter) {
        U32 link = l_iter->first;
        list<U32>* routers = l_iter->second;
        ASSERT(routers->size() > 0 && routers->size() <= 2);        

        //Each link can be connected to one or two routers only
        //knowing one end is useless, so handle only if both are known
        if (routers->size() == 2) {
            list<U32>::iterator r_iter = routers->begin();
            U32 first_router = *r_iter;
            r_iter++;
            U32 second_router = *r_iter;

            U32 link_cost = g_link_cost.find(link)->second;
            LINK first_edge = { second_router, link, link_cost };
            LINK second_edge = { first_router, link, link_cost };

            graph[first_router].insert(graph[first_router].end(), first_edge);
            graph[second_router].insert(graph[second_router].end(), second_edge);

            r_iter++;
            ASSERT(r_iter == routers->end());
        }
    }

    //print graph for debugging
    #if 0
    for (unsigned int i=1; i<TOTAL_ROUTERS + 1; ++i) {
        LOG("ROUTER GRAPH\n");
        LOG("Router: %i -> ", i);
        for(list<LINK>::iterator r_iter = graph[i].begin(); r_iter != graph[i].end(); ++r_iter) {
            LOG(" r%i/l%i/c%i ", r_iter->router_id, r_iter->link_id, r_iter->cost);
        }
        LOG("\n");
    }
    #endif
    
    for (U32 i=0; i<TOTAL_ROUTERS; ++i) {
        g_rib_path.insert(pair<U32, U32>(i+1, UNKNOWN_ROUTER));
        g_rib_cost.insert(pair<U32, U32>(i+1, MAX_COST));
    }

    //DJIKISTRA ALGORITHM
    U32 distance[TOTAL_ROUTERS + 1] = { MAX_COST, MAX_COST, MAX_COST, MAX_COST, MAX_COST, MAX_COST };
    bool visited[TOTAL_ROUTERS + 1] = { false };
    U32 last_sending_hop[TOTAL_ROUTERS + 1] = { 0 };
    
    priority_queue<LINK, std::vector<LINK>, compare> distance_queue;
    LINK first_router = { g_routerId, 0, 0 };
    distance_queue.push( first_router );

    distance[g_routerId] = 0;
    last_sending_hop[g_routerId] = g_routerId;


    LOG("Running DJIKISTRA Algorithm\n");
    while(false == distance_queue.empty()) {
        U32 router = distance_queue.top().router_id;
        distance_queue.pop();

        visited[router] = true;
        list<LINK> neighbors = graph[router];

        for (list<LINK>::iterator n_iter = neighbors.begin(); n_iter != neighbors.end(); n_iter++) {
            if ( ((distance[router] + n_iter->cost ) < distance[n_iter->router_id]) && (false == visited[n_iter->router_id]) )  {
                distance[n_iter->router_id] = distance[router] + n_iter->cost;
                last_sending_hop[n_iter->router_id] = router;

                distance_queue.push( *n_iter );
            }
        }        
    }

    //initialize rib
    g_rib_path.clear();
    g_rib_cost.clear();

    //Update RIB
    LOG("Updating RIB\n");
    for (U32 i=1; i < TOTAL_ROUTERS+1; ++i) {
        g_rib_cost.insert(pair<U32, U32>(i, distance[i]));

        list<U32> hops_list;
        U32 hop = last_sending_hop[i];
        if (hop == g_routerId) { //then it must be neighbor
            g_rib_path.insert(pair<U32, U32>(i, i));
        } else {
            while (hop != g_routerId) {
                hops_list.insert(hops_list.begin(), hop);
                hop = last_sending_hop[hop];

                //important .. in order to skip routers for which data is not available yet
                if (hop == 0) {
                    break;
                }
            }
            ASSERT(hops_list.size() > 0);
            g_rib_path.insert(pair<U32, U32>(i, *(hops_list.begin())));
        }
    }

    print_rib_info();
}

int main(int argc, char* argv[]) {
    //parse args
    if (argc != 5) {
        std::cout << "This program requires 4 arguments in the following format!" << std::endl;
        std::cout << "router <router id> <host address of emulator> <port of emulator> <router port>" << std::endl;
        std::cout << "Example: router 1 192.168.89.78 49998 11234" << std::endl;
        return 1;
    }

    g_routerId = atoi(argv[1]);
    g_remoteIp = inet_addr(argv[2]);
    g_remotePort = atoi(argv[3]);
    U32 localPort = atoi(argv[4]);

    g_socket.open(localPort);

    stringstream ss; 
    ss << "router" << g_routerId << ".log";
    file.open(ss.str().c_str(), std::ofstream::out | std::ofstream::app);
    if (false == file.is_open()) {
        LOG("LOG file cannot be opened. Exiting.\n");
    }

    sendInitPacket();

    initialize();

    char circuit_buffer[1024];
    unsigned long rIp; U32 rP;
    g_socket.read(circuit_buffer, 1024, rIp, rP);

    struct circuit_DB* circuit_db = (struct circuit_DB *) circuit_buffer;
    LOG("Received circuit database \n");
    for(U32 i=0; i<circuit_db->num_links; ++i) {
        g_link_cost.insert(pair<int, U32>(circuit_db->linkcost[i].link, circuit_db->linkcost[i].cost));
        LOG("Link : %i, Cost: %i \n", circuit_db->linkcost[i].link, circuit_db->linkcost[i].cost);
    }

    //Now that we have circuit DB. We send hello to neighbor packets
    for (U32 i=0; i<circuit_db->num_links; ++i) {
        int link_id = circuit_db->linkcost[i].link;
        sendHelloPacket(link_id);
    }

    //Recv all hello packets and create a map of participating routers
    while (true) {
        char recvBuffer[512];
        int readBytes = g_socket.read(recvBuffer, 512, rIp, rP);

        if (8 == readBytes) { //hello packet received
            //should update the map of neighbors
            struct pkt_HELLO* helloPacket = (struct pkt_HELLO*) recvBuffer;

            handleHelloPacket(helloPacket, circuit_db);
            
            //After hello send PDU
            sendDBToNeighbor(helloPacket->router_id, circuit_db);

        } else if (20 == readBytes) {    //if not the received packet is a PDU
            struct pkt_LSPDU* pduPacket = (struct pkt_LSPDU*) recvBuffer;
    
            bool result = handlePDUPacket(pduPacket);
            if (result) {
                updateRouterDatabase();
            }
        }
    }
    
    g_socket.close();
    file.flush();
    file.close();
    return 0;
}