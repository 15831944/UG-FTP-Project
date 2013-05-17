/* 
 * 
*/
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
#include "replyCode.h"
#include "requestqueue.h"
#include "util.h"
using namespace std;

map<string, string> accounts;
int  _debug_mode = 0;
#define SENDBUF_SIZE 1024
#define MAXSOCK		20
#define MAX_LINE	1024
#define MAX_FILESIZE 1024 * 1024 * 256	//256MB
#define SERVER_VERSION 1

string baseAddr;
void replyClient(int socket);

int main( int argc, char **argv) {
	if (argc < 4){
		printf("usage: server port accountFile homedir [-d]");
		exit(-1);
	}
	if (argc == 5 && argv[4][0] == '-' && argv[4][1] == 'd'){
		_debug_mode = 1;
	}
	baseAddr.assign(argv[3]);
	FILE* accFile = fopen(argv[2], "r");
	if (!accFile){
		printf("can not open account file");
		exit(-1);
	}
	char line[1000];
	char username[100], password[100];
	while ( fgets(line, 1000, accFile) != NULL){
		accounts[string("xxr3376")] = string("ug920801");
		sscanf("%s %s\n", username, password);
		accounts[string(username)] = string(password);
	}
	struct addrinfo hints, *res, *res0;
	int error;
	struct sockaddr_storage from;
	socklen_t fromlen;
	int s;
	int ls[MAXSOCK];
	int smax;
	int sockmax;
	fd_set rfd, rfd0;
	int n;
	int i;
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	char buf[MAX_LINE];
	/*
#ifdef IPV6_V6ONLY
	const int on = 1;
#endif
*/
	memset (&hints, 0, sizeof (hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	error = getaddrinfo (NULL, argv[1], &hints, &res0);
	if (error)
	{
		fprintf (stderr, "%s: %s\n", argv[1], gai_strerror (error));
		exit (1);
	}
	smax = 0;
	sockmax = -1;
	for (res = res0; res && smax < MAXSOCK; res = res->ai_next)
	{
		ls[smax] = socket (res->ai_family, res->ai_socktype,
				res->ai_protocol);
		if (ls[smax] < 0)
			continue;
		if (ls[smax] == FD_SETSIZE)
		{
			close (ls[smax]);
			ls[smax] = -1;
			continue;
		}
		/*
#ifdef IPV6_V6ONLY
		if (res->ai_family == AF_INET6 &&
		    setsockopt (ls[smax], IPPROTO_IPV6, IPV6_V6ONLY, &on,
				sizeof (on)) < 0)
		{
			perror ("bind");
			ls[smax] = -1;
			continue;
		}
#endif
*/
		int optval;
		if (setsockopt(ls[smax], SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0){
			perror("bind");
			ls[smax] = -1;
			continue;
		}
		
		if (bind (ls[smax], res->ai_addr, res->ai_addrlen) < 0)
		{
			close (ls[smax]);
			ls[smax] = -1;
			continue;
		}
		if (listen (ls[smax], 5) < 0)
		{
			close (ls[smax]);
			ls[smax] = -1;
			continue;
		}
		error = getnameinfo (res->ai_addr, res->ai_addrlen, hbuf,
				sizeof (hbuf), sbuf, sizeof (sbuf),
				NI_NUMERICHOST | NI_NUMERICSERV);
		if (error)
		{
			fprintf (stderr, "test: %s\n", gai_strerror (error));
			exit (1);
		}
		fprintf (stderr, "listen to %s %s\n", hbuf, sbuf);
		if (ls[smax] > sockmax)
			sockmax = ls[smax];
		smax++;
	}
	freeaddrinfo (res0);
	if (smax == 0)
	{
		fprintf (stderr, "test: no socket to listen to\n");
		exit (1);
	}
	FD_ZERO (&rfd0);
	for (i = 0; i < smax; i++)
		FD_SET (ls[i], &rfd0);
	while (1)
	{
		rfd = rfd0;
		n = select (sockmax + 1, &rfd, NULL, NULL, NULL);
		if (n < 0)
		{
			perror ("select");
			exit (1);
		}
		for (i = 0; i < smax; i++)
		{
			if (FD_ISSET (ls[i], &rfd))
			{
				fromlen = sizeof (from);
				s = accept (ls[i], (struct sockaddr *) &from, &fromlen);
				if (s < 0)
					continue;
				pid_t pid = fork();
				if (pid != 0){
					// son
					printf("Thread %d is sereving\n", pid);
					replyClient(s);
					close(s);
					printf("Thread %d finish\n", pid);
					exit(0);
				}
			}
		}
	}
	for (i = 0; i < smax; i++)
		close (ls[i]);
}
static char welcomeMsg[] = "(ugFTPd 1.0)";
struct Client{
	LoginStatus loginStatus;
	string username;
	string cur_path;
	int dataSocket;
	int dataStatus;
	int ctrlSocket;
	RequestQueue* rq;
	struct addrinfo hints, *res;
	char* sendBuf;
	Client(int cs, char* buf, RequestQueue* r){
		ctrlSocket = cs;
		loginStatus = LOGIN_INIT;
		dataStatus = 0;
		dataSocket = -1;
		sendBuf = buf;
		rq = r;
	}
};

void _send(int socket, const char* buf, int l){
	if (_debug_mode){
		printf("<<<%s\n", buf);
	}
	write(socket, buf, l);
	return;
}

int connectPASV(Client* client){
	if (client->dataSocket != -1){
		close(client->dataSocket);
		client->dataSocket = -1;
	}
	int &datafd = client->dataSocket;
	datafd = socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in dataaddr, daddr, caddr;
	dataaddr.sin_family = AF_INET;
	dataaddr.sin_addr.s_addr = INADDR_ANY;
	dataaddr.sin_port = 0;

	if  (bind(datafd, (struct sockaddr *)&dataaddr, sizeof(dataaddr))<0) {
		printf("bind error!");
		return -1;
	}
	if (listen(datafd, 1) < 0) {
		printf("listen error!");
		return -1;
	}
	socklen_t length  =   sizeof (daddr);
	if (getsockname(datafd,(struct sockaddr*)&daddr, &length) <0) {
		printf("getsockname error!");
		return -1;
	}
	getsockname(client->ctrlSocket, (struct sockaddr*)&caddr, &length);

	listen(datafd, 4);
	int addr = caddr.sin_addr.s_addr;
	int port = ntohs(daddr.sin_port);
	int sendLength = sprintf(client->sendBuf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n", (addr&0xFF), ((addr&0xFF00)>>8), ((addr&0xFF0000)>>16), ((addr&0xFF000000)>>24), (port>>8), (port&0xFF));
	_send(client->ctrlSocket, client->sendBuf, sendLength);
	client->dataStatus = 1;
	return 0;
}

int connectPORT(Client* client, string data){
	int &datafd = client->dataSocket;
	if (datafd != -1) {
		close(datafd);
		datafd = -1;
	}
	char host[100];
	char port[100];
	int h1, h2, h3, h4, p1, p2;
	sscanf(data.c_str(), "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
	sprintf(host, "%d.%d.%d.%d", h1, h2, h3, h4);
	sprintf(port, "%d", p1 * 256 + p2);
	ssize_t l;
	int error;
	struct addrinfo& hints = client->hints;
	struct addrinfo *& res = client->res;
/* check the number of arguments */
	memset (&hints, 0, sizeof (hints));
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo (host, port, &hints, &res);
	if (error) {
		fprintf (stderr, "%s %s: %s\n", host, port, gai_strerror (error));
		exit (1);
	}
	/*
	datafd = socket (res->ai_family, res->ai_socktype, res->ai_protocol);
	if (datafd < 0)
		return -1;
	if (connect (datafd , res->ai_addr, res->ai_addrlen) < 0)
	{
		close (datafd);
		return -1;
	}
	*/
	int sendLength = sprintf(client->sendBuf, "200 PORT command successful. Consider using PASV.\r\n");
	_send(client->ctrlSocket, client->sendBuf, sendLength);
	client->dataStatus = 1;
	return 0;
}

int openDataPort(Client* client){
	if (!(client->dataSocket)){
		return -1;
	}
	if (!(client->dataSocket > 0)){
		struct addrinfo& hints = client->hints;
		struct addrinfo *& res = client->res;
		int &datafd = client->dataSocket;
		datafd = socket (res->ai_family, res->ai_socktype, res->ai_protocol);
		if (datafd < 0)
			return -1;
		if (connect (datafd , res->ai_addr, res->ai_addrlen) < 0)
		{
			close (datafd);
			return -1;
		}
		return 0;
	}
	else{
		int data_client_fd = -1;
		struct sockaddr_in data_client_addr;
		int size=sizeof(struct sockaddr_in);
		data_client_fd = accept(client->dataSocket, (struct sockaddr *) & data_client_addr, (socklen_t*) &size);
		close(client->dataSocket);
		client->dataSocket = data_client_fd;
	}
	return 0;
}	

void replyClient(int socket){
	chdir(baseAddr.c_str());
	RequestQueue* rq = new RequestQueue(_debug_mode, socket);
	char sendBuf[SENDBUF_SIZE + 1];
	Client client(socket, sendBuf, rq);
	int sendLength;
	char current_path[SENDBUF_SIZE + 1];
	
	sendLength = sprintf(sendBuf, "%d %s\r\n", StdReply::SERVER_READY, welcomeMsg);
	_send(socket, sendBuf, sendLength);

	while(true){
		Request req = rq->getNew();
		if (req.req == "QUIT"){
			sendLength = sprintf(sendBuf, "221 Goodbye.\r\n");
			_send(socket, sendBuf, sendLength);
			printf("[LOG]user: %s log out\n", client.username.c_str());
			break;
		}
		else if (req.req == "USER"){
			if (client.loginStatus == LOGIN_LOGINED){
				// 230 Already logged in.
				sendLength = sprintf(sendBuf, "%d Already logged in.\r\n", StdReply::LOGIN_SUCCESS);
				_send(socket, sendBuf, sendLength);
			}
			else{
				client.username = req.data;
				client.loginStatus = LOGIN_INPUT_USERNAME;
				// 331 Please specify the password.
				sendLength = sprintf(sendBuf, "%d Please specify the password.\r\n", StdReply::RIGHT_USERNAME);
				_send(socket, sendBuf, sendLength);
			}
			continue;
		}
		else if (req.req == "PASS"){
			if (client.loginStatus == LOGIN_INPUT_USERNAME){
				//check accounts
				if (accounts.find(client.username) != accounts.end()){
					if (accounts[client.username] == req.data){
						//230 Login successful.
						client.loginStatus = LOGIN_LOGINED;
						sendLength = sprintf(sendBuf, "%d Login successful.\r\n", StdReply::LOGIN_SUCCESS);
						_send(socket, sendBuf, sendLength);
						printf("[LOG]user: %s log in\n", client.username.c_str());
						continue;
					}
				}
				//530 Login incorrect.
				sendLength = sprintf(sendBuf, "%d Login incorrect.\r\n", StdReply::LOGIN_INCORRECT);
				_send(socket, sendBuf, sendLength);
			}
			else if (client.loginStatus == LOGIN_INIT){
				//503 Login with USER first.
				sendLength = sprintf(sendBuf, "%d Login with USER first.\r\n", StdReply::USERNAME_FIRST);
				_send(socket, sendBuf, sendLength);
			}
			else if (client.loginStatus == LOGIN_LOGINED){
				//230 Already logged in.
				sendLength = sprintf(sendBuf, "%d Already logged in.\r\n", StdReply::LOGIN_SUCCESS);
				_send(socket, sendBuf, sendLength);
			}
			continue;
		}
		else{
			if (client.loginStatus != LOGIN_LOGINED){
				//530 Please login with USER and PASS.
				sendLength = sprintf(sendBuf, "%d Please login with USER and PASS.\r\n", StdReply::LOGIN_INCORRECT);
				_send(socket, sendBuf, sendLength);
				continue;
			}
			// already logged in
			if (req.req == "LIST"){
				if (!client.dataStatus){
					// 425 Use PORT or PASV first.
					sendLength = sprintf(sendBuf, "425 Use PORT or PASV first.\r\n");
					_send(socket, sendBuf, sendLength);
				}
				else{
					sendLength = sprintf(sendBuf, "150 Here comes the directory listing.\r\n");
					_send(socket, sendBuf, sendLength);
					openDataPort(&client);
					FILE* lsdata = popen("ls -l | tail -n +2", "r");
					
					sendFile( lsdata, client.dataSocket);
					close(client.dataSocket);
					client.dataSocket = -1;
					client.dataStatus = 0;

					sendLength = sprintf(sendBuf, "226 Directory send OK.\r\n");
					_send(socket, sendBuf, sendLength);
					pclose(lsdata);
				}
				continue;
			}
			else if (req.req == "PWD"){
				getcwd(current_path, SENDBUF_SIZE);
				sendLength = sprintf(sendBuf, "257 \"%s\" is current dictionary.\r\n", current_path);
				_send(socket, sendBuf, sendLength);
				continue;
			}
			else if (req.req == "CWD"){
				if (chdir(req.data.c_str()) == 0){
					sendLength = sprintf(sendBuf, "250 Directory successfully changed.\r\n");
				}
				else{
					sendLength = sprintf(sendBuf, "550 Failed to change directory.\r\n");
				}
				_send(socket, sendBuf, sendLength);
				continue;
			}
			else if (req.req == "PASV"){
				connectPASV(&client);
				continue;
			}
			else if (req.req == "PORT"){
				connectPORT(&client, req.data);
				continue;
			}
			else if (req.req == "STOR"){
				if (!client.dataStatus){
					// 425 Use PORT or PASV first.
					sendLength = sprintf(sendBuf, "425 Use PORT or PASV first.\r\n");
					_send(socket, sendBuf, sendLength);
				}
				else{
					FILE* fdata = fopen(req.data.c_str(), "wb");
					if (!fdata){
						// 550 Failed to open file. 
						sendLength = sprintf(sendBuf, "550 Failed to open file.\r\n");
						_send(socket, sendBuf, sendLength);
						continue;
					}

					sendLength = sprintf(sendBuf, "150 Ok to send data.\r\n");
					_send(socket, sendBuf, sendLength);
					openDataPort(&client);
					
					recvFile( fdata, client.dataSocket);
					close(client.dataSocket);
					client.dataSocket = -1;
					client.dataStatus = 0;

					sendLength = sprintf(sendBuf, "226 Transfer complete.\r\n");
					_send(socket, sendBuf, sendLength);
					fclose(fdata);
				}
				continue;
			}
			else if (req.req == "RETR"){
				if (!client.dataStatus){
					// 425 Use PORT or PASV first.
					sendLength = sprintf(sendBuf, "425 Use PORT or PASV first.\r\n");
					_send(socket, sendBuf, sendLength);
				}
				else{
					FILE* fdata = fopen(req.data.c_str(), "rb");
					if (!fdata){
						// 550 Failed to open file. 
						sendLength = sprintf(sendBuf, "550 Failed to open file.\r\n");
						_send(socket, sendBuf, sendLength);
						continue;
					}

					sendLength = sprintf(sendBuf, "150 Opening BINARY mode data connection.\r\n");
					_send(socket, sendBuf, sendLength);
					openDataPort(&client);
					
					sendFile( fdata, client.dataSocket);
					close(client.dataSocket);
					client.dataSocket = -1;
					client.dataStatus = 0;

					sendLength = sprintf(sendBuf, "226 Transfer complete.\r\n");
					_send(socket, sendBuf, sendLength);
					fclose(fdata);
				}

				continue;
			}
			else if (req.req == "TYPE"){
				if (req.data == "I"){
					//200 Switching to Binary mode.
					sendLength = sprintf(sendBuf, "200 Switching to Binary mode.\r\n");
					_send(socket, sendBuf, sendLength);
				}
				else{
					//504 Command not implemented for that parameter.
					sendLength = sprintf(sendBuf, "504 Command not implemented for that parameter.\r\n");
					_send(socket, sendBuf, sendLength);
				}
				continue;
			}
			else if (req.req == "SYST"){
				// 215 UNIX Type: L8
				sendLength = sprintf(sendBuf, "%d UNIX Type: L8\r\n", StdReply::NAME_SYSTEM_TYPE );
				_send(socket, sendBuf, sendLength);
				continue;
			}
			// 502 Command not implemented.
			sendLength = sprintf(sendBuf, "%d Command not implemented.\r\n", StdReply::COMMAND_NOT_IMPLEMENTED );
			_send(socket, sendBuf, sendLength);
			break;
		}
	}
	return;
}

