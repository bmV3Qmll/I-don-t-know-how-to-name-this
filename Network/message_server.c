#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "robust_IO.h" 
#include "socket_wrapper.h"

#define MAXLEN 1024

typedef void (*sighandler_t)(int);

pid_t Fork(){
    pid_t pid = fork();
    if (pid < 0){
        perror("fork error");
        exit(1);
    }
    return pid;
}

sighandler_t Signal(int signum, sighandler_t handler){
    sighandler_t prev = signal(signum, handler);
    if (prev == SIG_ERR){
        perror("signal error");
        exit(1);
    }
    return prev;
}

void echo(int connfd){
	struct rio rp;
	rio_init(&rp, connfd);
	char buf[MAXLEN];
	size_t n;
	
	while((n = buf_readline(&rp, buf, MAXLEN)) != 0){
		writen(connfd, buf, strlen(buf));
	}
	return;
}

void sigchld_handler(int sig){
	while(waitpid(-1, 0, WNOHANG) > 0);
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
	
	struct sockaddr client;
	socklen_t addrlen;
	char host[MAXLEN], port[MAXLEN];
	
	char * welcome = "Welcome to the boring server which repeats whatever it received. Let's start!\n";
	
	Signal(SIGCHLD, sigchld_handler);

	while(1){
		addrlen = sizeof(struct sockaddr);
		connfd = accept(listenfd, &client, &addrlen);
		if (getnameinfo(&client, addrlen, host, MAXLEN, port, MAXLEN, 0) == 0){
			printf("Receive connection from %s:%s\n", host, port);
			if (Fork() == 0){
				close(listenfd);
				writen(connfd, welcome, strlen(welcome));
				echo(connfd);
				close(connfd);
				exit(0);
			}
		}
		close(connfd);
	}
	
	return 0;
}
