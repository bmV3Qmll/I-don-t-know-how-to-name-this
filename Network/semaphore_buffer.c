#include <semaphore.h>
#include <stdlib.h>
#include "semaphore_buf.h"

/*  Structure to store available file descriptors that:
    - pushed in by main thread
    - queued by peer threads to perform operations on
    - protected by semaphore
    Use producer-consumer model
*/
typedef struct{
    int n;              // number of allocated slots
    int * buf;          // array of fds
    int head;
    int tail;           // all fds whose index from (head + 1) to tail are ready
    sem_t mutex;        // protect access to buf
    sem_t slots;        // indicating number of empty slots
    sem_t items;
} sem_buf;

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
    ret = p->buf[(p->head++) % (p->n)];
    sem_post(&p->mutex);
    sem_post(&p->slots);
    return ret;
}
