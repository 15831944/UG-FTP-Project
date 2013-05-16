all: client

client: client.cpp replyCode.h commandqueue.o
	g++ $^ -o $@
commandqueue.o: commandqueue.h commandqueue.cpp
