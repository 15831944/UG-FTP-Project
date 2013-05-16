#ifndef _REQUESTQUEUE_H_
#define _REQUESTQUEUE_H_
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string>
#include <queue>
#include "replyCode.h"
using namespace std;
#define RECVBUF_SIZE 1024

class Request{
public:
	string req;
	string data;
};
class RequestQueue{
	int debug_mode;
	int socket;
	queue<Request > q;
	char recvBuf[RECVBUF_SIZE + 1];
	int length;
	void _recv();
	void _processData();
public:
	// should init with a socket
	RequestQueue(int debug_mode, int s);
	Request getNew();
};
#endif /* _REQUESTQUEUE_H_ */

