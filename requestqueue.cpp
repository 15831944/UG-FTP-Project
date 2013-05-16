#include "requestqueue.h"
#include "replyCode.h"
#include <cstdlib>
#include <queue>
#include <algorithm>
using namespace std;

void RequestQueue::_recv(){
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

RequestQueue::RequestQueue(int d, int s){
	this->debug_mode = d;
	this->socket = s;
}

Request RequestQueue::getNew(){
	if (q.size()){
		Request r = q.front();
		q.pop();
		return r;
	}
	else{
		_recv();
		_processData();
		if (q.size()){
			Request r = q.front();
			q.pop();
			return r;
		}
		else{
			exit(-1);
		}
	}
}

void RequestQueue::_processData(){
	int lastStart = 0;
	int pointer = 0;
	while ( pointer < length){
		while( recvBuf[pointer] == ' ') pointer++;
		lastStart = pointer;
		while( recvBuf[pointer] != ' ' && recvBuf[pointer] != '\r' && recvBuf[pointer] != '\n' &&  pointer < length)  pointer++;
		recvBuf[pointer] = '\0';

		string req(recvBuf + lastStart);
		std::transform(req.begin(), req.end(), req.begin(), ::toupper);
		lastStart = ++pointer;
		while( ( pointer < length) && ( recvBuf [pointer] != '\r' || recvBuf[pointer +1] != '\n')){
			pointer += 1;
		}
		string data;
		if (pointer > lastStart){
			recvBuf[pointer - 2] = '\0';
			data = string(recvBuf + lastStart);
		}
		Request temp;
		temp.req = req;
		temp.data = data;
		q.push(temp);

		pointer += 2;
	}
	length = 0;
}

