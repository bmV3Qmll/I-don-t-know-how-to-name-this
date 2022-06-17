#include <sys/type.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "robust_IO.h" 
#include "socket_wrapper.h"

#define MAXLEN 1024

int main(int argc, char * argv[]){
	if (argc != 3){
		fprintf(stderr, "usage: %s <hostname> <port>\n", argv[0]);
		exit(0);
	}
	
	char buf[MAXLEN];
	int fd = open_clientfd(argv[1], argv[2]);
	if (fd == -1){
		fprintf(stderr, "404: Address not found");
		exit(1);
	}
	
	struct rio * rp;
	rio_init(rp, fd);
	
	buf_readline(rp, buf, MAXLEN);
	fputs(buf, stdout);
	
	while(fgets(buf, MAXLEN, stdin) != NULL){
		writen(fd, buf, strlen(buf));
		buf_readline(rp, buf, MAXLEN);
		fputs(buf, stdout);
	}
	close(fd);
	return 0;
}
