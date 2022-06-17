#include <sys/type.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

int Getaddrinfo(const char *restrict node, const char *restrict service, const struct addrinfo *restrict hints, struct addrinfo **restrict res){
	int err;
	if ((err = getaddrinfo(node, service, hints, res)) != 0){
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(err));
		exit(1);
	}
	return 0;
}

int open_clientfd(char * host, char * port){
	struct addrinfo *hints, **list_addr, *iter;
	int clientfd;
	
	memset(hints, 0, sizeof(struct addrinfo));
	hints->ai_socktype = SOCK_STREAM;
	hints->ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;
  
	Getaddrinfo(host, port, hints, list_addr);
	
	for (iter = *list_addr; iter; iter = iter->ai_next){
		if((clientfd = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol)) == -1){continue;}
		if (connect(clientfd, iter->ai_addr, iter->ai_addrlen) == 0){break;}
		close(clientfd); 	//	unable to connect
	}
	
	freeaddrinfo(list_addr);
	if (!iter){	//	fail to create socket / connect to all addresses
		return -1;
	}
	return clientfd;
}

int open_listenfd(char * port){
	struct addrinfo *hints, **list_addr, *iter;
	int listenfd, optval = 1;
	
	memset(hints, 0, sizeof(struct addrinfo));
	hints->ai_socktype = SOCK_STREAM;
	hints->ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG | AI_PASSIVE;
  
	Getaddrinfo(NULL, port, hints, list_addr);
	
	for (iter = *list_addr; iter; iter = iter->ai_next){
		if ((listenfd = socket(iter->ai_family, iter->ai_socktype, iter->ai_protocol)) == -1){continue;}
		
		//	Eliminates "Address already in use" error from bind
		Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
		
		//	Bind the descriptor to the address
		if (bind(listenfd, iter->ai_addr, iter->ai_addrlen) == 0){break;}
		close(listenfd);
	}
	
	freeaddrinfo(list_addr);
	if (!iter){return -1;}
	
	//	 Make a listening socket ready to accept connection requests
	if (listen(listenfd, 1024) == 0){return listenfd;}
	close(listenfd);
	return -1;
}
