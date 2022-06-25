#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include "robust_IO.h" 
#include "socket_wrapper.h"
#include "semaphore_buf.h"

#define MAXLEN 1024
#define MAXPEER 4

sem_buf sbuf;
static int byte_cnt;
static sem_t mutex;        // protect access to byte_cnt

static void echo_init(void){
    byte_cnt = 0;
    sem_init(&mutex, 0, 1);
}

void thread_echo(int fd){
    int nread;
    rio dio;
    char buf[MAXLEN];
    static pthread_once_t once = PTHREAD_ONCE_INIT;

    pthread_once(&once, echo_init);
    rio_init(&dio, fd);

    while((nread = buf_readline(&dio, buf, MAXLEN)) > 0){
        sem_wait(&mutex);
        byte_cnt += nread;
        printf("Receive %d more bytes. Total: %d bytes\n", nread, byte_cnt);
        sem_post(&mutex);
        writen(fd, buf, nread);
    }
}

void *thread(void *vargp){
    pthread_detach(pthread_self());
    while (1){
        int connfd = sbuf_remove(&sbuf);
        thread_echo(connfd);
        close(connfd);
    }
}

int main(int argc, char * argv[]){
    int listenfd , connfd;
    pthread_t tid;
    struct sockaddr client;
    socklen_t addrlen;

    if (argc != 2){
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }

    sbuf_init(&sbuf, MAXPEER);

    if ((listenfd = open_listenfd(argv[1])) == -1){
	fprintf(stderr, "unable to open port %s", argv[1]);
	exit(1);
    }

    for (int i = 0; i < MAXPEER; ++i){
        pthread_create(&tid, NULL, &thread, NULL);
    }

    while(1){
        addrlen = sizeof(struct sockaddr);
	connfd = accept(listenfd, &client, &addrlen);
        sbuf_insert(&sbuf, connfd);
    }
}
