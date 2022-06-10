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
    int pid;
    int jid;
    job * next;
    char * desc;
};

job * construct(int p, int j, job * prev, char * des){
    job * t = Malloc(sizeof(job));
    t->pid = p;
    t->jid = j;
    if (prev){prev->next = t;} // jobs->head don't have prev pointer
    t->desc = des;
}

// linked_list contains two empty end nodes head && tail; count to keep track of the number of jobs
struct linked_list{
    int count;
    job * head;
    job * tail;
} * jobs;
/*
	in main implementation, declare jobs as follow:
	jobs = (struct linked_list *) Malloc(sizeof(struct linked_list));
    jobs->count = 0;
    jobs->head = construct(2367, 10, NULL, NULL);
    jobs->tail = construct(2879, 11, jobs->head, NULL);
*/

job * push(int p, char * des){ // O(1)
    job * newTail = construct(0, 0, NULL, NULL);
    jobs->tail->pid = p;
    jobs->tail->jid = ++jobs->count;
    jobs->tail->next = newTail;
    jobs->tail->desc = des;
    jobs->tail = newTail;
}

void erase(int p){ // O(n)
    if (!jobs->count){return;}
    job * iter = jobs->head;
    for (; iter != jobs->tail; iter = iter->next){
        if (iter->next->pid == p){
            iter->next = iter->next->next;
            iter = iter->next;
            --jobs->count;
            break;
        }
    }
    for (; iter != jobs->tail; iter = iter->next){
        --iter->jid;
    }
}


void print-jobs(int fd){ // O(n)
    job * iter = jobs.head->next;
    while (; iter != jobs.tail; iter = iter->next){
        dprintf(fd, "[%d] %d %s &\n", iter->jid, iter->pid, iter->desc);
    }
}
/*
debugging code

int main(){
    jobs = (struct linked_list *) Malloc(sizeof(struct linked_list));
    jobs->count = 0;
    jobs->head = construct(2367, 10, NULL, NULL);
    jobs->tail = construct(2879, 11, jobs->head, NULL);
    push(1189, "cat hello.txt | sort");
    job * sec = push(2205, "ls -la");
    push(3456, "strace test.c");
    erase(1189);
    print_jobs();
    return 0;
}
*/
