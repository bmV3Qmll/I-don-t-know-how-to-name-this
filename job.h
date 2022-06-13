#ifndef JOB_H_
#define JOB_H_

typedef struct job job;
typedef struct linked_list ll;
void * Malloc(size_t);
struct job{
    pid_t pid;
    int jid;
    job * next;
    char * desc;
};

//volatile int count;

struct linked_list{
    job * head;
    job * tail;
};

job * construct(pid_t, int, job*, char*);
void push(ll *, pid_t, char *);
job * search(ll *, int, pid_t);
void erase(job *);
void print_job(job *, int);
void list_jobs(ll *, int);
void free_all(ll *);

#endif
