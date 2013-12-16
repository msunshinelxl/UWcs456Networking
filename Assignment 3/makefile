CC = g++
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LDFLAGS = -Wall $(DEBUG)

ROUTER_SOURCES = router.cpp UDPSocket.cpp
ROUTER_OBJECTS = $(ROUTER_SOURCES:.cpp=.o)
ROUTER_EXECUTABLE = router

all: $(ROUTER_SOURCES) $(ROUTER_EXECUTABLE)
	
$(ROUTER_EXECUTABLE): $(ROUTER_OBJECTS) 
	$(CC) $(LDFLAGS) $(ROUTER_OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *o $(ROUTER_EXECUTABLE).