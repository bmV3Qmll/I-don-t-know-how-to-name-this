#include <semaphore.h>
#include <stdlib.h>
#include "semaphore_buf.h"

void sbuf_init(sem_buf * p, int n){
    p->buf = calloc(n, sizeof(int));
    p->n = n;
    p->head = p->tail = 0;
    sem_init(&p->mutex, 0, 1);
    sem_init(&p->slots, 0, n);
    sem_init(&p->items, 0, 0);
}

void sbuf_clean(sem_buf * p){
    free(p->buf);
}

void sbuf_insert(sem_buf * p, int fd){
    sem_wait(&p->slots);
    sem_wait(&p->mutex);
    p->buf[(++p->tail) % (p->n)] = fd;
    sem_post(&p->mutex);
    sem_post(&p->items);
}

// remove and return first fd from sem_buf to be served by peer thread
int sbuf_remove(sem_buf * p){
    int ret;
    sem_wait(&p->items);
    sem_wait(&p->mutex);
    ret = p->buf[(++p->head) % (p->n)];
    sem_post(&p->mutex);
    sem_post(&p->slots);
    return ret;
}
