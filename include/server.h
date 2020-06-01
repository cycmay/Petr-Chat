#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include "protocol.h"
#include "ClientLinkList.h" 

#define BUFFER_SIZE 		1024
#define QUEUE_SIZE			1024 
#define ADDR_LEN 			sizeof(struct sockaddr)
#define SA 					struct sockaddr
#define CNTNT_LEN			150
#define USER_LEN			20

// message in queue 
typedef struct MessageData{
	enum 	msg_types type; 
	int 	pid;
	char 	from_username[USER_LEN];
	char 	to_username[USER_LEN];
	char 	mText[BUFFER_SIZE];
	int  	clientfd;
}MessageData;

#define MSGDATA_LEN			sizeof(MessageData)
#define SPETATOR			"\r\n"
#define ROOMLIMIT			5

// share structure message queue
void run_server(int server_port, int jobNum);
int _send_msg(int clientfd, enum msg_types type, char *body);
// 系统删除用户信息
void removeClientBySystem(int clientfd);
#endif
