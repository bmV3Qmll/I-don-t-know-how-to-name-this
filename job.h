#ifndef JOB_H_
#define JOB_H_

typedef struct job job;

void * Malloc(size_t);
struct job{
    pid_t pid;
    int jid;
    job * next;
    char * desc;
};

volatile int count;

struct linked_list{
    job * head;
    job * tail;
} * jobs;

job * construct(pid_t, int, job*, char*);
job * push(pid_t, char *);
job * search(int, pid_t);
void erase(job *);
void print_job(job *, int);
void list_jobs(int);

#endif
