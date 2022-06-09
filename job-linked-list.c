#include <stdlib.h>
#include <stdio.h>

#ifndef MAX_LEN
#define MAX_LEN 128
#endif

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
  char * desc[MAX_LEN];
};

job * construct(pid_t p = 0, int j = 0, job * prev = NULL, char * des = NULL){
  job * t = Malloc(sizeof(job));
  t->pid = p;
  t->jid = j;
  prev->next = t;
  t->desc = dec;
}

// linked-list contains two empty end nodes head && tail; count to keep track of the number of jobs
struct linked-list{
  int count = 0;
  job * head = construct();
  job * tail = construct(0, 0, head);
};

linked-list jobs{};

job * push(pid_t p, char * dec){ // O(1)
  job * newTail = construct();
  jobs.tail->pid = p;
  jobs.tail->jid = ++jobs.count;
  jobs.tail->next = newTail;
  jobs.tail->desc = dec;
  jobs.tail = newTail;
}

void erase(pid_t p){ // O(n)
  if (!jobs.count){return;}
  job * iter = jobs.head;
  while (; iter != jobs.tail; iter = iter->next){
    if (iter->next->pid == p){
      iter->next = iter->next->next;
      iter = iter->next;
      --jobs.count;
      break;
    }
  }
  while (; iter != jobs.tail; iter = iter->next){
    --iter->jid;
  }
}

void print-jobs(int fd){ // O(n)
  job * iter = jobs.head->next;
  while (; iter != jobs.tail; iter = iter->next){
    dprintf(fd, "[%d] %d %s &\n", iter->jid, iter->pid, iter->desc);
  }
}
