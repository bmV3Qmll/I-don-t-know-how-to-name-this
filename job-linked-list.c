#include <stdlib.h>
#include <stdio.h>

typedef struct job job;

void * Malloc(size_t size){
    void * res = malloc(size);
    if (res){return res;}
    perror("malloc error");
    exit(1);
}

struct job{
    pid_t pid;
    int jid;
    job * next;
    char * desc;
};

job * construct(pid_t p, int j, job * prev, char * des){
    job * t = Malloc(sizeof(job));
    t->pid = p;
    t->jid = j;
    if (prev){prev->next = t;} // jobs->head don't have prev pointer
    t->desc = des;
}

volatile int count = 0;

// linked_list contains two empty end nodes head && tail; count to keep track of the number of jobs
struct linked_list{
    job * head;
    job * tail;
} * jobs;
/*
    in main implementation, declare jobs as follow:
    jobs = (struct linked_list *) Malloc(sizeof(struct linked_list));
    jobs->head = construct(0, 0, NULL, NULL);
    jobs->tail = construct(0, 0, jobs->head, NULL);
*/

job * push(pid_t p, char * des){ // O(1)
    job * newTail = construct(0, 0, NULL, NULL);
    jobs->tail->pid = p;
    jobs->tail->jid = ++count;
    jobs->tail->next = newTail;
    jobs->tail->desc = des;
    jobs->tail = newTail;
}

void erase(pid_t p){ // O(n)
    job * iter = jobs->head;
    for (; iter != jobs->tail; iter = iter->next){
        if (iter->next->pid == p){
            job * temp = iter->next;
            iter->next = iter->next->next;
            free(temp);
            iter = iter->next;
            break;
        }
    }
}

void print_jobs(int fd){ // O(n)
    job * iter = jobs->head->next;
    for (; iter != jobs->tail; iter = iter->next){
        dprintf(fd, "[%d] %d %s &\n", iter->jid, iter->pid, iter->desc);
    }
}
/*
debugging code

int main(){
    jobs = (struct linked_list *) Malloc(sizeof(struct linked_list));
    jobs->head = construct(0, 0, NULL, NULL);
    jobs->tail = construct(0, 0, jobs->head, NULL);
    push(1189, "cat hello.txt | sort");
    job * sec = push(2205, "ls -la");
    push(3456, "strace test.c");
    erase(1189);
    print_jobs(1);
    return 0;
}
*/
