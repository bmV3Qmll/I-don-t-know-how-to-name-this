#ifndef JOB_H_
#define JOB_H_

void * Malloc(size_t);
typedef struct job{
    pid_t pid;
    int jid;
    job * next;
    char * desc;
} job;

volatile int count;

struct linked_list{
    job * head;
    job * tail;
} * jobs;

job * construct(pid_t, int, job*, char*);
job * push(pid_t, char *);
void erase(pid_t);
void print_jobs(int);

#endif
