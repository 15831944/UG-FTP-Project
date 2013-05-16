all: client server
.PHONY: clean

client: client.cpp replyCode.h commandqueue.o
	g++ $^ -o $@
server: server.cpp replyCode.h requestqueue.o util.h
	g++ $^ -o $@
commandqueue.o: commandqueue.h commandqueue.cpp
requestqueue.o: requestqueue.h requestqueue.cpp

clean:
	rm *.o
	rm client
	rm server
