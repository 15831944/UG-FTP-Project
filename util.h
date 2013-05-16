#ifndef _UTIL_H_
#define _UTIL_H_

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
#include <map>

#define BUFFER_SIZE 1024
char buf[BUFFER_SIZE];

int sendFile(FILE * file, int datafd)
{
	int l = 0;
	while ((l = fread(buf, 1, BUFFER_SIZE, file)) > 0){
		write(datafd, buf, l);
	}
}
int recvFile(FILE * file, int datafd)
{
	int l = 0;
	while ((l = read(datafd, buf, BUFFER_SIZE)) > 0){
		fwrite(buf, 1, l, file);
	}
}

#endif /* _UTIL_H_ */
