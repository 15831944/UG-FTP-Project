/* 
 * ugFTP client 1.0
 * Author: ugeeker<xxr3376@gmail.com>
 * Date: 2013-5-16
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
#include "replyCode.h"
#include "commandqueue.h"
#include <algorithm>
#include <string>
#include "util.h"

using namespace std;

// GLOBAL SOCKET DATA
#define SENDBUF_SIZE 1024
#define DATA_BUF_SIZE 10240
int Scontrol;
int Sdata;

int _debug_mode = 0;
int dataMode = 0;	//0 mean passive, 1 mean 主动

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
	Scontrol = socket (res->ai_family, res->ai_socktype, res->ai_protocol);
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

//return 0 if login success, else state Code
int serverlogin(){
	ReplyCode command;
	int sendLength;

	command = cq->getNewCommand();
	if (command != StdReply::SERVER_READY){
		badEnd("server send wrong hello message");
	}

	printf("welcome to ugFTP client v1.0\n");
	char temp[100];
	printf("ftp> username: ");
	fgets(temp, 100, stdin);
	int l = strlen(temp) - 1;
	while (temp[l] == '\r' || temp[l] == '\n'){
		temp[l--] = '\0';
	}
	sendLength = sprintf(sendBuf, "USER %s\r\n", temp);
	if (_debug_mode){
		printf("<<<%s", sendBuf);
	}
	write(Scontrol, sendBuf, sendLength);
	command = cq->getNewCommand();
	if (command != StdReply::RIGHT_USERNAME){
		badEnd("server reject username");
	}
	printf("ftp> password: ");
	fgets(temp, 100, stdin);
	system("clear");
	l = strlen(temp) - 1;
	while (temp[l] == '\r' || temp[l] == '\n'){
		temp[l--] = '\0';
	}
	sendLength = sprintf(sendBuf, "PASS %s\r\n", temp);
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
int connectPASSIVE(){
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
// PORT
int connectPORT(){
	int &datafd = Sdata;
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
	getsockname(Scontrol, (struct sockaddr*)&caddr, &length);
	listen(datafd, 4);
	int addr = caddr.sin_addr.s_addr;
	int port = ntohs(daddr.sin_port);
	int sendLength = sprintf(sendBuf, "PORT %d,%d,%d,%d,%d,%d\r\n", (addr&0xFF), ((addr&0xFF00)>>8), ((addr&0xFF0000)>>16), ((addr&0xFF000000)>>24), (port>>8), (port&0xFF));
	write(Scontrol, sendBuf, sendLength);
	if (cq->getNewCommand() != 200){
		return -1;
	}
	return 0;
}
//PASSIVE
int openDataPort(){
	if (dataMode == 0){
		//PASV
		return connectPASSIVE();
	}
	else {
		//PORT
		return connectPORT();
	}
}
int action_list(){
	if (openDataPort()){
		badEnd("data port fail");
		return 0;
	}
	int sendLength = sprintf(sendBuf, "LIST\r\n");
	write(Scontrol, sendBuf, sendLength);
	if (dataMode){
		int data_client_fd = -1;
		struct sockaddr_in data_client_addr;
		int size=sizeof(struct sockaddr_in);
		data_client_fd = accept(Sdata, (struct sockaddr *) & data_client_addr, (socklen_t*) &size);
		close(Sdata);
		Sdata = data_client_fd;
	}	
	ReplyCode command = cq->getNewCommand();
	if ( !( command == StdReply::TRANSFERING || command == StdReply::OPENING_DATAPORT)){
		return -1;
	}
	recvFile(stdout, Sdata);
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
	FILE* files = fopen(filename, "wb");
	if (!files){
		printf("can not open local file\n");
		return -1;
	}
	if (openDataPort()){
		badEnd("data port fail");
		return 0;
	}
	int sendLength = sprintf(sendBuf, "RETR %s\r\n", filename);
	write(Scontrol, sendBuf, sendLength);
	if (dataMode){
		int data_client_fd = -1;
		struct sockaddr_in data_client_addr;
		int size=sizeof(struct sockaddr_in);
		data_client_fd = accept(Sdata, (struct sockaddr *) & data_client_addr, (socklen_t*) &size);
		close(Sdata);
		Sdata = data_client_fd;
	}	
	ReplyCode command = cq->getNewCommand();
	recvFile(files, Sdata);
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
	if (dataMode){
		int data_client_fd = -1;
		struct sockaddr_in data_client_addr;
		int size=sizeof(struct sockaddr_in);
		data_client_fd = accept(Sdata, (struct sockaddr *) & data_client_addr, (socklen_t*) &size);
		close(Sdata);
		Sdata = data_client_fd;
	}	
	command = cq->getNewCommand();
	sendFile(files, Sdata);
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
	if (serverlogin()){
		badEnd("login fail");
		return;
	}
	char userInputBuf[1000];
	while(true){
		printf("\nftp> ");
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
		else if (ui.op == "cls" || ui.op == "clear"){
			system("clear");
		}
		else if (ui.op == "debug"){
			_debug_mode = 1 - _debug_mode;
			cq->setDebugMode(_debug_mode);
			printf("debug mode = %d\n", _debug_mode);
		}
		else if (ui.op == "passive"){
			dataMode = 0;
			printf("passive mode is on");
		}
		else if (ui.op == "port"){
			dataMode = 1;
			printf("initiative mode is on");
		}
		else if (ui.op == "help" || ui.op == "?"){
			printf("\ndir | list | ls: \tList files on server\n");
			printf("pwd: \t\t\tShow current path\n");
			printf("cd: \t\t\tChange current path\n");
			printf("put: \t\t\tUpload file\n");
			printf("get: \t\t\tdownload file\n");
			printf("bye | quit : \t\tQuit ftp\n");
			printf("cls | clear : \t\tClear Screen\n");
			printf("debug: \t\t\tSwitch Debug Mode\n");
			printf("passive: \t\tSet to use passive mode\n");
			printf("port: \t\t\tSet to use initiative mode\n");
			printf("help | ?: \t\tShow Help\n");
		}
	}
}

int main ( int argc, char *argv[])
{
	if (argc < 2){
		printf("usage: client domainName [port] [-d]");
		exit(-1);
	}
	char port[100] = "21";
	if (argc == 3){
		strcpy(port, argv[2]);
	}
	if (argc == 4 && argv[3][0] == '-' && argv[3][1] == 'd'){
		_debug_mode = 1;
	}
	if(initConnection(argv[1], port)){
		printf("can not connect, EXIT\n");
		exit(-1);
	}
	cq = new CommandQueue(_debug_mode, Scontrol);
	ftpLoop();
	closeConnection();
}

