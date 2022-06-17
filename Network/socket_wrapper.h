#ifndef SOCKET_WRAPPER_H_
#define SOCKET_WRAPPER_H_

int Getaddrinfo(const char *restrict, const char *restrict, const struct addrinfo *restrict, struct addrinfo **restrict);
int open_clientfd(char *, char *);
int open_listenfd(char *, char *);

#endif
