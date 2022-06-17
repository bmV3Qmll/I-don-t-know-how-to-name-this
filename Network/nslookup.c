/*	My immplementation of nslookup written in C 
	Desire product should query a string of hostname and return all possible addresses
*/
#include <sys/type.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXLEN 128

int main(int argc, char * argv[]){
	if (argc != 2){
		fprintf(stderr, "usage: %s <domain name>\n", argv[0]);
		exit(0);
	}
	
	struct addrinfo *hints, **list_addr;
	int err, flag = NI_NUMERICHOST;
	char buf[MAXLEN];
	
	//	configure hints
	memset(hints, 0, sizeof(struct addrinfo));
	hints->ai_family = AF_UNSPEC;
	hints->ai_socktype = SOCK_STREAM;
	
	if ((err = getaddrinfo(argv[1], NULL, hints, list_addr)) != 0){
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(err));
		exit(1);
	}
	
	for (struct addrinfo * iter = *list_addr; iter; iter = iter->ai_next){
		if((getnameinfo(iter->ai_addr, iter->ai_addrlen, buf, MAXLEN, NULL, 0, flag)) == 0){
			fprintf(stdout, "Address: %s\n", buf);
		}
	}
	
	//	clean up
	freeaddrinfo(list_addr);
	
	return 0;
}
