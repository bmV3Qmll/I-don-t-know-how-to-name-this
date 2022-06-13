/* The following code is a trivial implementation of Unix shell, features:
- Prompt user for input
- Run built-in commands, which are indicated in "built-in" function
- Run executable files with specified arguments and set environment variables (supports only absolute path)
- Handle SIGINT / SIGSTP 
- Jobs control
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include "job.h"

#define MAXLEN 64
#define MAXARGS 16
#define MAXJOBS 32

volatite pid_t fg_pid; // indicating which process is running in foreground

//  syscalls plus error checking 
pid_t Fork(){
    pid_t pid = fork();
    if (pid < 0){
        perror("fork error");
        exit(1);
    }
    return pid;
}

sighandler_t Signal(int signum, sighandler_t handler){
    if (signal(signum, handler) == SIG_ERR){
        perror("signal error");
        exit(1);    
    }
}

int Kill(pid_t pid, int sig){
    if (kill(pid, sig) < 0){
        perror("kill error");
        return -1;
    }
    return 0;
}

int Sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oldset){
    int ret = sigprocmask(how, set, oldset);
    if (ret < 0){
        perror("sigprocmask error");
        exit(1);
    }
    return ret;
}

/*  This function parses arguemnts to argv and returns whether user want to run in background or not
    buf is a modifiable copy of input command 
*/
int parseline(char * buf, char * argv[]){
    char *delim;
    int argc = 0, bg;
    
    buf[strlen(buf) - 1] = ' '; // replace '\n' by ' ' for the while loop
    while (*buf && *buf == ' '){++buf;} // ignore leading spaces
    
    while ((delim = strchr(buf, 0x20))){
        argv[argc++] = buf; 
        *delim = '\0'; // terminate argument by null byte
        
        buf = delim + 1;
        while (*buf && *buf == ' '){++buf;} // ignore spaces between arguments
    }
    argv[argc] = NULL;
    if ((bg = (argv[argc - 1] == '&'))){
        argv[--argc] = NULL;
    }
    return bg;
}

//  Execute pre-defined commands (if called)
int built_in(char * argv[]){
    if (argv[0] == "exit"){exit(0);}
    else if (argv[0] == "jobs"){list_jobs(1); return 1;}
    else if (argv[0] != "bg" && argv[0] != "fg"){return 0;}
    
    //  Process JID / PID
    int j = -1;
    pid_t p = -1;
    if (*argv[1] == '%'){j = atoi(argv[1] + 1);}
    else{p = atoi(argv[1]);}

    if (!j || !p){
        perror("Invalid ID");
        return 0;
    }
    
    int fg = !strncmp(argv[0], (char *) "fg", 2);
    int stat;
    p = waitpid(p, &stat, WNOHANG);
    if (p && WIFSTOPPED(stat)){
        // if process stopped, send SIGCONT
        Kill(p, SIGCONT);
        if (fg){waitpid(p, &stat, 0);}
    }else if (p){
        //  process terminated but hadn't been erased
        //  fork new one
        job * tar = search(j, p)->next;
        if (!tar){return 1;}    // 404
        char * des = tar->desc;
        if (fg){ // des ends with '   &   \n' -> fix this
            char * temp = strchr(des, '&');
            *temp = '\0';
        }
        eval(des);
        //  old process will be deleted by sigchild_handler
    }// else p still runs
    return 1;
}


// These are signal handlers for SIGINT, SIGCHILD and SIGSTP
void sigint_handler(int sig){
    Kill(fg_pid, SIGINT); // Sends SIGINT to foreground process
    return;
}

void sigchild_handler(int sig){
    pid_t child_pid;
    int old_errno = errno;  // save errno so that following function don't interfere other parts that rely on errno
    sigset_t full_mask, prev_mask;
    sigfillset(&full_mask);
    while((child_pid = waitpid(-1, NULL, 0)) > 0){
        // Protect accesses to shared global data structure (jobs) by blocking all signals
        Sigprocmask(SIG_BLOCK, &full_mask, &prev_mask);
        erase(search(-1, child_pid));
        // log something maybe
        Sigprocmask(SIG_SETMASK, &prev_mask, NULL);
    }
    if (errno != ECHILD){
        perror("waitpid error");
    }
    errno = old_errno;  // restore errno
}

void sigstp_handler(int sig){
    Kill(fg_pid, SIGSTP); // Sends SIGSTP to foreground process
    return;
}

// This function executes user command
void eval(char * cmd){
    char * argv[MAXARGS];
    char buf[MAXLEN];
    int bg;
    pid_t pid;
    
    // Initialize masks
    sigset_t full_mask, child_mask, prev_child;
    sigfillset(&full_mask);
    sigemptyset(&child_mask);
    sigaddset(&child_mask, SIGCHILD);
    
    // Register signal handler
    Signal(SIGINT, sigint_handler); 
    Signal(SIGCHILD, sigchild_handler);
    Signal(SIGSTP, sigstp_handler);
    
    strncpy(buf, cmd, strlen(cmd));
    bg = parseline(buf, argv);
    
    if (argv[0] == NULL){return;} // Empty command line
    
    if (!built_in(argv)){
/* Mitigate race condition
If the child runs in background, it must be added to job lists, and when it terminated, erase will be called by sigchild_handler.
However, the following sequences can lead to error:
1. Child terminates before context switches to parent, in which SIGCHILD is sent to parent
2. sigchild_handler is triggered, and call erase, which deletes the non-existence job
3. After that, parent call push to add the terminated job to lists

To prevent this, the parent must pre-block SIGCHILD until push is called
-> Synchronizing Flows to Avoid Concurrency Bugs
*/
        // fork and get child process execve the executable file
        Sigprocmask(SIG_BLOCK, &child_mask, &prev_child); // block SIGCHILD
        if ((pid = Fork()) == 0){
            Sigprocmask(SIG_SETMASK, &prev_child, NULL); // child process -> unblock SIGCHILD 
            if (execve(argv[0], argv, environ) < 0){
                perror("execve error");
                exit(1);
            }
        }
        
        if (bg){
            printf("%d: %s", pid, cmd);
            // Protect accesses to shared global data structure (jobs) by blocking all signals
            Sigprocmask(SIG_BLOCK, &full_mask, NULL);
            push(pid, cmd);
        }else{
            fg_pid = pid;
            int stat;
            if (waitpid(pid, &stat, 0) < 0){
                perror("waitpid error");
            }
        }
        Sigprocmask(SIG_SETMASK, &prev_child, NULL); // reset signal mask
    }
    return;
}

int main(){
    char cmd[MAXLEN];
    
    // Initialize job manager
    jobs = (struct linked_list *) Malloc(sizeof(struct linked_list));
    jobs->head = construct(0, 0, NULL, NULL);
    jobs->tail = construct(0, 0, jobs->head, NULL);
    
    while (1){
        printf("$ ");
        fgets(cmd, MAXLEN, STDIN_FILENO);
        if (feof(STDIN_FILENO)){exit(0);} // detects EOF signal
        
        eval(cmd);
    }
    return 0;
}
