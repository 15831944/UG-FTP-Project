#include "commandqueue.h"
#include "replyCode.h"
#include <cstdlib>
#include <queue>
using namespace std;

void CommandQueue::_recvCommand(){
	int bufOffset = 0;
	int l = 0;
	while ((l = read (socket, recvBuf + bufOffset, RECVBUF_SIZE - bufOffset)) > 0){
		if (recvBuf[ l + bufOffset - 1 ] == '\n'){
			recvBuf[ l + bufOffset - 1] = '\0';
			break;
		}
		bufOffset += l;
	}
	if (debug_mode){
		printf(">>>%s\n", recvBuf);
	}
	length = l + bufOffset;
	return;
}

CommandQueue::CommandQueue(int d, int s){
	this->debug_mode = d;
	this->socket = s;
}

ReplyCode CommandQueue::getNewCommand(){
	if (q.size()){
		ReplyCode r = (q.front()).code;
		q.pop();
		return r;
	}
	else{
		_recvCommand();
		_processData();
		if (q.size()){
			ReplyCode r = (q.front()).code;
			q.pop();
			return r;
		}
		else{
			exit(-1);
		}
	}
}
Command CommandQueue::getNewOriginal(){
	if (q.size()){
		Command r = q.front();
		q.pop();
		return r;
	}
	else{
		_recvCommand();
		_processData();
		if (q.size()){
			Command r = q.front();
			q.pop();
			return r;
		}
		else{
			exit(-1);
		}
	}
}


void CommandQueue::_processData(){
	int lastStart = 0;
	int pointer = 0;
	ReplyCode command;
	while ( pointer < length){
		while ((pointer < length) && (recvBuf[pointer] > '9' || recvBuf[pointer] < '0')){
			pointer++;
		}
		ReplyCode command = genCode(recvBuf[pointer], recvBuf[pointer + 1], recvBuf[pointer + 2]);
		lastStart = pointer;
		pointer += 3;
		while( ( pointer < length) && ( recvBuf [pointer] != '\r' || recvBuf[pointer +1] != '\n')){
			pointer += 1;
		}
		recvBuf[pointer] = '\0';
		pointer += 2;
		string total(recvBuf + lastStart);
		Command temp;
		temp.code = command;
		temp.original = total;
		q.push(temp);
	}
	length = 0;
}

