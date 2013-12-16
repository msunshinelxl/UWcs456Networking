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
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <string>
#define ASSERT(x) assert(x)

#define LOG(...) printf( __VA_ARGS__ )

const unsigned int WINDOW_SIZE = 10;
const unsigned int TIMER_SECS = 2;

const int SENDER_UDP_PORT = 11234;
const int RECEIVER_UDP_PORT = 11235;

const int EMULATOR_SENDER_TO_RECEIVER_PORT = 49998;
const int EMULATOR_RECEIVER_TO_SENDER_PORT = 49999;

typedef int SOCKET;

#endif //SOCKET_H
