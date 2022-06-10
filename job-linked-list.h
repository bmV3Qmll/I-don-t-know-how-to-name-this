#ifndef JOB_H_
#define JOB_H_

void * Malloc(size_t);
struct job;
job * construct(pid_t, int, job*, char*);
struct linked_list;
job * push(pid_t, char *);
void erase(pid_t);
void print_jobs(int);

#endif
