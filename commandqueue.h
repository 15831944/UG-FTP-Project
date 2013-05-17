#ifndef _COMMANDQUEUE_H_
#define _COMMANDQUEUE_H_
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

struct Command{
	ReplyCode code;
	string original;
};
class CommandQueue{
	int debug_mode;
	int socket;
	queue<Command > q;
	char recvBuf[RECVBUF_SIZE + 1];
	int length;
	void _recvCommand();
	void _processData();
public:
	// should init with a socket
	CommandQueue(int debug_mode, int s);
	ReplyCode getNewCommand();
	Command getNewOriginal();
	void setDebugMode(int a);
};
#endif /* _COMMANDQUEUE_H_ */

