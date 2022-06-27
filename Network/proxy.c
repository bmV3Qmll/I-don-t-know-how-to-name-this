#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include "robust_IO.h" 
#include "socket_wrapper.h"
#include "semaphore_buf.h"

#define MAXLEN 1024
#define MAXRESP 2048
#define CACHE_LIMIT 8192
#define DEF_MODE S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH
#define TIMEOUT 5.0

int logfd;      // log's file descriptor
char * filter;  // pointer to readable virtual memory mapping of filter file
sem_t mutex;    // prevent logfd from simultaneous write operation
const char * methods = "POST PUT PATCH";
struct stat st; // stat of filter file
typedef void (*sighandler_t)(int);

void * Malloc(size_t size){
    void * res = malloc(size);
    if (res){return res;}
    perror("malloc error");
    exit(1);
}

sighandler_t Signal(int signum, sighandler_t handler){
    sighandler_t prev = signal(signum, handler);
    if (prev == SIG_ERR){
        perror("signal error");
        exit(1);
    }
    return prev;
}

void sigint_handler(int sig){
    munmap(filter, st.st_size);
    pthread_exit(0);
}

void error(int fd, char * cause, int code, char * msg, char * desc){
    char buf[MAXLEN], body[MAXRESP];

    sprintf(body, "<html><title>Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%d: %s\r\n", body, code, msg);
    sprintf(body, "%s<p>%s: %s\r\n", body, desc, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
    
    sprintf(buf, "HTTP/1.0 %d %s\r\n", code, msg);
    writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    writen(fd, buf, strlen(buf));
    writen(fd, body, strlen(body));
}

void process_hdrs(rio * rp, char * cache, char * lim, char * host, char * port, int * len){
    char buf[MAXLEN];
    int nread;
    while ((nread = buf_readline(rp, buf, MAXLEN)) > 0){
        if (!strncmp(buf, "Host: ", 6)){
            host = buf + 6;
            port = strchr(host, ':');
            if (port){
                *port = '\0';
                ++port;
            }
        }
        if (!strncmp(buf, "Content-Length: ", 16)){
            *len = strtol(buf + 16, 0, 10);
        }
        else if (!strcmp(buf, "Transfer-Encoding: chunked\r\n")){
            *len = 0;
        }
        sprintf(cache, "%s", buf);
        cache += nread;
        if (!strcmp(buf, "\r\n")){break;}
    }
}

int allow_url(char url[]){
    if (strstr(filter, url) == NULL){return 1;}
    return 0;
}

int get_body(int forward_fd, rio * rp, char * cache, char * lim, int len){
    int nread, chunked;
    char buf[MAXLEN];
    if (!len){chunked = 1;}
    while(1){
        if (chunked){
            buf_readline(rp, buf, MAXLEN);
            len = strtol(buf, 0, 16);
            if (!len){break;}
        }
        if (len > lim - cache){
            error(forward_fd, "Cache overloaded", 413, "Payload Too Large", "Request entity is larger than limits");
            return 0;
        }
        nread = buf_readn(rp, cache, len);
        if (nread == -1){
            error(forward_fd, "Broken pipe", 500, "Internal Server Error", "");
            return 0;
        }
        if (nread < len){
            error(forward_fd, "Missing body data", 400, "Bad Request", "Retry");
            return 0;
        }
        if (!chunked){break;}
    }
    return 1;
}

int forward(int fd){
    struct rio conn_io;
    char * cache, * host, * port, * head, * end;
    char method[16], uri[64], version[16], url[128], buf[128];
    int clientfd, body = -1, nread;

    rio_init(&conn_io, fd);
    cache = Malloc(CACHE_LIMIT);
    head = cache;
    end = head + CACHE_LIMIT;
    host = NULL;

    // get uri
    nread = buf_readline(&conn_io, buf, MAXLEN);
    sscanf(buf, "%s %s %s", method, uri, version);
    sprintf(cache, "%s", buf);
    cache += nread;

    process_hdrs(&conn_io, cache, end, host, port, &body);

    if (host == NULL){
        error(fd, "Host", 400, "Bad Request", "Missing request header");
        free(cache);
        return 0;
    }

    // assemble the url
    strcpy(url, host);
    if (port){
        strcat(url, ":");
        strcat(url, port);
    }
    strcat(url, uri);
    strcat(url, "\n");

    sem_post(&mutex);
    writen(logfd, url, strlen(url));
    sem_wait(&mutex);

    if (!allow_url(url)){
        error(fd, url, 403, "Forbidden", "In blacklist");
        free(cache);
        return 0;
    }

    // get body
    if (strstr(methods, method) != NULL){
        if (body == -1){
            error(fd, method, 411, "Length Required", "Content-Length is required for");
            free(cache);
            return 0;
        }
        if(!get_body(fd, &conn_io, cache, end, body)){
            free(cache);
            return 0;
        }
    }

    // send requests
    if ((clientfd = open_clientfd(host, port)) == -1){
        error(fd, host, 502, "Bad Gateway", "Unable to connect");
        free(cache);
        return 0;
    }
    writen(clientfd, head, cache - head);
    free(cache);
    return clientfd;
}

void reply(int client, int server){
    int ready, body;
    struct timeval tval;
    fd_set set;
    char * cache, * head;
    rio server_io;

    tval.tv_sec = TIMEOUT;
    FD_ZERO(&set);
    FD_SET(server, &set);

    if (!(ready = select(server + 1, &set, NULL, NULL, &tval))){
        error(client, "5.0 secs", 504, "Gateway Timeout", "Destination server did not response in");
        return;
    }else if (ready == -1){
        error(client, "", 500, "Internal Server Error", "");
        return;
    }

    cache = Malloc(CACHE_LIMIT);
    head = cache;
    rio_init(&server_io, server);

    process_hdrs(&server_io, cache, cache + CACHE_LIMIT, NULL, NULL, &body);

    if (!get_body(client, &server_io, cache, cache + CACHE_LIMIT, body)){
        free(cache);
        return;
    }
    writen(client, head, cache - head);
    free(cache);
}

void *thread(void *vargp){
    int connfd, server;
    connfd = *((int *) vargp);
    printf("%d\n", connfd);
    pthread_detach(pthread_self());
    free(vargp);
    if ((server = forward(connfd))){
	    printf("forward ok\n");
        reply(connfd, server);
		printf("reply ok\n");
    }
    close(connfd);
    exit(0);
}

int main(int argc, char * argv[]){
    int listenfd, * connfd, filterfd;
    struct sockaddr * client;
	socklen_t addrlen;
	char host[MAXLEN], port[MAXLEN];
    pthread_t tid;
    
    if (argc != 4){
        fprintf(stderr, "usage: %s <port> <log file> <filter file>\n", argv[0]);
		exit(0);
    }

    if ((listenfd = open_listenfd(argv[1])) == -1){
        fprintf(stderr, "unable to open port %s", argv[1]);
		exit(1);
    }

    if ((logfd = open(argv[2], O_WRONLY | O_APPEND, 0)) == -1){
        fprintf(stderr, "unable to open log file %s", argv[2]);
        exit(1);
    }
    sem_init(&mutex, 0, 1);

    if ((filterfd = open(argv[3], O_CREAT | O_RDONLY, DEF_MODE)) == -1){
        fprintf(stderr, "unable to open / create filter file %s", argv[3]);
        exit(1);
    }
    stat(argv[3], &st);
    if ((filter = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, filterfd, 0)) == (void *) -1){
        fprintf(stderr, "unable to map filter file %s", argv[3]);
        exit(1);
    }
    close(filterfd);

    Signal(SIGINT, sigint_handler);

    while(1){
        addrlen = sizeof(struct sockaddr);
        connfd = Malloc(sizeof(int));
        *connfd = accept(listenfd, client, &addrlen);
	// accept returns -1, generate "Bad address"
	/*
	if (getnameinfo(client, addrlen, host, MAXLEN, port, MAXLEN, 0) == 0){
            printf("Receive connection from %s:%s\n", host, port);
	    pthread_create(&tid, NULL, &thread, connfd);
	}*/
    }
}
