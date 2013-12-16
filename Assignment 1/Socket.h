/**
* This file has all include files for linux environment.
* It also has default socket ports.
*/

#ifndef SOCKET_H
#define SOCKET_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#define ASSERT(x) assert(x)
#define LOG(...) printf( __VA_ARGS__ )

const int SERVER_DEFAULT_TCP_PORT = 11234;

const int SERVER_DEFAULT_UDP_PORT = 49998;
const int CLIENT_DEFAULT_UDP_PORT = 49999;

typedef int SOCKET;

#endif //SOCKET_H
