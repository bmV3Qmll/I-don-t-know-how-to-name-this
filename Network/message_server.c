#include <sys/type.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "robust_IO.h" 
#include "socket_wrapper.h"

#define MAXLEN 1024

void echo(int connfd){
	struct rio * rp;
	rio_init(rp, connfd);
	char buf[MAXLEN];
	
	while(buf_readline(rp, buf, MAXLEN)){
		writen(connfd, buf, strlen(buf));
	}
	return;
}

int main(int argc, char * argv[]){
	if (argc != 2){
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}
	
	int listenfd = open_listenfd(argv[1]), connfd;
	if (listenfd == -1){
		fprintf(stderr, "unable to open port %s", argv[1]);
		exit(1);
	}
	
	struct sockaddr * client;
	socklen_t addrlen;
	char host[MAXLEN], port[MAXLEN];
	
	char * welcome = "Welcome to the boring server which repeats whatever it received. Let's start!";
	
	while(1){
		addrlen = sizeof(struct sockaddr);
		connfd = accept(listenfd, client, &addrlen);
		if (getnameinfo(client, addrlen, host, MAXLEN, port, MAXLEN, 0) == 0){
			printf("Receive connection from %s:%s\n", host, port);
			writen(connfd, welcome, strlen(welcome));
			echo(connfd);
		}
		close(connfd);
	}
	
	return 0;
}