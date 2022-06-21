#ifndef SEM_BUF_H
#define SEM_BUF_H

typedef struct{
    int n;
    int * buf;
    int head;
    int tail;
    sem_t mutex;
    sem_t slots;
    sem_t items;
}sem_buf;

void sbuf_init(sem_buf *, int);
void sbuf_clean(sem_buf *);
void sbuf_insert(sem_buf *, int);
int sbuf_remove(sem_buf *);

#endif