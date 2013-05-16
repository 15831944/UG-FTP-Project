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
#include "replyCode.h"
#include "commandqueue.h"
#include <algorithm>
#include <string>

using namespace std;

// GLOBAL SOCKET DATA
#define SENDBUF_SIZE 1024
#define DATA_BUF_SIZE 10240
int Scontrol;
int Sdata;

int _debug_mode = 0;

struct addrinfo *res0;
struct addrinfo *res1;
CommandQueue* cq = NULL;

char sendBuf[SENDBUF_SIZE + 1];
char dataRecvBuf[DATA_BUF_SIZE + 1];
char dataSendBuf[DATA_BUF_SIZE + 1];

void badEnd(const char* message){
	fprintf(stderr, "ERROR!: %s, EXIT\n", message);
	exit(-1);
}

int initConnection(const char* host, const char* port){
	struct addrinfo hints, *res;
	ssize_t l;
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	int error;
/* check the number of arguments */
	memset (&hints, 0, sizeof (hints));
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo (host, port, &hints, &res0);
	if (error)
	{
		fprintf (stderr, "%s %s: %s\n", host, port,
			 gai_strerror (error));
		exit (1);
	}
	res = res0;
	error = getnameinfo (res->ai_addr, res->ai_addrlen, hbuf,
			sizeof (hbuf), sbuf, sizeof (sbuf),
			NI_NUMERICHOST | NI_NUMERICSERV);
	if (error)
	{
		fprintf (stderr, "%s %s: %s\n", host, port,
				gai_strerror (error));
		return -1;
	}
	if (_debug_mode){
		fprintf (stderr, "trying %s port %s\n", hbuf, sbuf);
	}
	Scontrol = socket (res->ai_family, res->ai_socktype,
			res->ai_protocol);
	if (Scontrol < 0)
		return -1;
	if (connect (Scontrol, res->ai_addr, res->ai_addrlen) < 0)
	{
		close (Scontrol);
		Scontrol = -1;
		return -1;
	}
	return 0;
}

void closeConnection(){
	close (Scontrol);
	freeaddrinfo (res0);
	close (Sdata);
	freeaddrinfo (res1);
}


int recvData(FILE* target){
	int l = 0;
	while ((l = read (Sdata, dataRecvBuf, DATA_BUF_SIZE)) > 0){
		fwrite(dataRecvBuf, 1, l, target);
		if (_debug_mode){
			fwrite(dataRecvBuf, 1, l, stdout);
		}
	}
	return 0;
}
int sendData(FILE* source){
	int l = 0;
	while ((l = fread(dataSendBuf, 1, DATA_BUF_SIZE, source)) > 0){
		write(Sdata, dataSendBuf, l);
	}
	return 0;
}


//return 0 if login success, else state Code
int serverlogin(const char* username, const char* password){
	ReplyCode command;
	int sendLength;

	command = cq->getNewCommand();
	if (command != StdReply::SERVER_READY){
		badEnd("server send wrong hello message");
	}
	sendLength = sprintf(sendBuf, "USER %s\r\n", username);
	if (_debug_mode){
		printf("<<<%s", sendBuf);
	}
	write(Scontrol, sendBuf, sendLength);
	command = cq->getNewCommand();
	if (command != StdReply::RIGHT_USERNAME){
		badEnd("server reject username");
	}
	sendLength = sprintf(sendBuf, "PASS %s\r\n", password);
	if (_debug_mode){
		printf("<<<PASS XXXX\n", sendBuf);
	}
	write(Scontrol, sendBuf, sendLength);
	command = cq->getNewCommand();
	if (command == StdReply::LOGIN_SUCCESS){
		// success
		return 0;
	}
	// failed
	return (int) command;
}
//PASSIVE
int openDataPort(){
	ReplyCode command;
	int sendLength;
	sendLength = sprintf(sendBuf, "PASV\r\n");
	write(Scontrol, sendBuf, sendLength);
	Command com = cq->getNewOriginal();
	if (com.code != StdReply::ENTER_PASSIVE){
		badEnd("server refuse to use passive");
	}
	char host[100];
	char port[100];
	int h1, h2, h3, h4, p1, p2;
	sscanf(com.original.c_str(), "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).", &h1, &h2, &h3, &h4, &p1, &p2);
	sprintf(host, "%d.%d.%d.%d", h1, h2, h3, h4);
	sprintf(port, "%d", p1 * 256 + p2);

	struct addrinfo hints, *res;
	ssize_t l;
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
	int error;
/* check the number of arguments */
	memset (&hints, 0, sizeof (hints));
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo (host, port, &hints, &res1);
	if (error)
	{
		fprintf (stderr, "%s %s: %s\n", host, port,
			 gai_strerror (error));
		exit (1);
	}
	res = res1;
	error = getnameinfo (res->ai_addr, res->ai_addrlen, hbuf,
			sizeof (hbuf), sbuf, sizeof (sbuf),
			NI_NUMERICHOST | NI_NUMERICSERV);
	if (error)
	{
		fprintf (stderr, "%s %s: %s\n", host, port,
				gai_strerror (error));
		return -1;
	}
	if (_debug_mode){
		fprintf (stderr, "trying %s port %s\n", hbuf, sbuf);
	}
	Sdata = socket (res->ai_family, res->ai_socktype,
			res->ai_protocol);
	fflush(stdout);
	if (Sdata < 0)
		return -1;
	if (connect (Sdata , res->ai_addr, res->ai_addrlen) < 0)
	{
		close (Sdata);
		Sdata = -1;
		return -1;
	}
	return 0;
}
int action_list(){
	if (openDataPort()){
		badEnd("data port fail");
		return 0;
	}
	int sendLength = sprintf(sendBuf, "LIST\r\n");
	write(Scontrol, sendBuf, sendLength);
	ReplyCode command = cq->getNewCommand();
	if ( !( command == StdReply::TRANSFERING || command == StdReply::OPENING_DATAPORT)){
		return -1;
	}
	recvData(stdout);
	// transfer OK FLAG
	command = cq->getNewCommand();
}
void action_pwd(){
	int sendLength;
	sendLength = sprintf(sendBuf, "PWD\r\n");
	write(Scontrol, sendBuf, sendLength);
	Command com = cq->getNewOriginal();
	printf("%s\n", com.original.c_str());
}
void action_cwd(const char* dir){
	int sendLength;
	sendLength = sprintf(sendBuf, "CWD %s\r\n", dir);
	write(Scontrol, sendBuf, sendLength);
	Command com = cq->getNewOriginal();
	printf("%s\n", com.original.c_str());
}
void action_bye(){
	int sendLength;
	sendLength = sprintf(sendBuf, "QUIT\r\n");
	write(Scontrol, sendBuf, sendLength);
	Command com = cq->getNewOriginal();
	printf("%s\n", com.original.c_str());
}
int action_download(const char* filename){
	if (openDataPort()){
		badEnd("data port fail");
		return 0;
	}
	int sendLength = sprintf(sendBuf, "RETR %s\r\n", filename);
	FILE* files = fopen(filename, "wb");
	if (!files){
		printf("can not open local file\n");
		return -1;
	}
	write(Scontrol, sendBuf, sendLength);
	ReplyCode command = cq->getNewCommand();
	recvData(files);
	fclose(files);
	// transfer OK FLAG
	command = cq->getNewCommand();
	return 0;
}
int action_upload(const char* filename){
	FILE* files = fopen(filename, "rb");
	if (!files){
		printf("can not open local file\n");
		return -1;
	}
	int sendLength = sprintf(sendBuf, "TYPE I\r\n");
	write(Scontrol, sendBuf, sendLength);
	ReplyCode command = cq->getNewCommand();
	if (command != 200){
		return -1;
	}
	if (openDataPort()){
		badEnd("data port fail");
		return 0;
	}
	sendLength = sprintf(sendBuf, "STOR %s\r\n", filename);
	write(Scontrol, sendBuf, sendLength);
	command = cq->getNewCommand();
	sendData(files);
	close (Sdata);
	fclose(files);
	// transfer OK FLAG
	command = cq->getNewCommand();
	return 0;
}
struct UserInput{
	string op;
	string data;
	UserInput(char* input){
		int n = strlen(input);
		int p = 0;
		while (p < n && (input[p] != ' ' && input[p] != '\n')){
			p++;
		}
		input[p] = '\0';
		op.assign(input);
		std::transform(op.begin(), op.end(), op.begin(), ::tolower);
		if (++p < n){
			while (input[p] == ' ') p++;
			while (input[n - 1] == '\n' || input[n - 1] == '\r'){
				input[n - 1] = '\0';
			}
			data.assign(input + p);
		}
	}
};

void ftpLoop(){
	if (serverlogin("xxr3376", "ug920801")){
		badEnd("login fail");
		return;
	}
	char userInputBuf[1000];
	while(true){
		printf("\n(ftp)> ");
		gets(userInputBuf); 
		UserInput ui(userInputBuf);
		if (ui.op == "dir" || ui.op == "list" || ui.op == "ls"){
			action_list();
		}
		else if (ui.op == "pwd"){
			action_pwd();
		}
		else if (ui.op == "cd"){
			action_cwd(ui.data.c_str());
		}
		else if (ui.op == "put"){
			action_upload(ui.data.c_str());
		}
		else if (ui.op == "get"){
			action_download(ui.data.c_str());
		}
		else if (ui.op == "bye" || ui.op == "quit"){
			action_bye();
			break;
		}
	}
}

int main ( int argc, char *argv[])
{
	initConnection("localhost", "2222");
	cq = new CommandQueue(_debug_mode, Scontrol);
	ftpLoop();
	closeConnection();
}

