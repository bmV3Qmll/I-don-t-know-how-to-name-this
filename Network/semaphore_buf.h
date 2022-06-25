#ifndef SEM_BUF_H
#define SEM_BUF_H

/*  Structure to store available file descriptors that:
    - pushed in by main thread
    - queued by peer threads to perform operations on
    - protected by semaphore
    Use producer-consumer model
*/
struct sem_buf {
    int n;              // number of allocated slots
    int * buf;          // array of fds
    int head;
    int tail;           // all fds whose index from (head + 1) to tail are ready
    sem_t mutex;        // protect access to buf
    sem_t slots;        // indicating number of empty slots
    sem_t items;
};

typedef struct sem_buf sem_buf;

void sbuf_init(sem_buf *, int);
void sbuf_clean(sem_buf *);
void sbuf_insert(sem_buf *, int);
int sbuf_remove(sem_buf *);

#endif
