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

#define MAXLEN 64
#define MAXARGS 16
#define MAXJOBS 8

pid_t jobs[MAXJOBS], cnt = 0;

volatite pid_t fg_pid; // indicating which process is running in foreground

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

int built_in(char * argv[]){
  switch(argv[0]){
    case "exit":
      exit(0);
    case "jobs":
      for (int i = 0; i < cnt; ++i){
        printf("%d runs in background\n", jobs[i]);
      }
      break;
    case "bg":
      
      break;
    case "fg":
      
      break;
    default:
      return 0;
  }
}


// These are signal handler for SIGINT, SIGCHILD and SIGSTP
void sigint_handler(int sig){
  Kill(fg_pid, SIGINT); // Sends SIGINT to foreground process
  return;
}

void sigchild_handler(int sig){
  pid_t child_pid
  while((child_pid = waitpid(-1, NULL, 0)) > 0){
    
  }
}

// This function executes user command
void eval(char * cmd){
  char * argv[MAXARGS];
  char buf[MAXLEN];
  int bg;
  pid_t pid;
  
  strcpy(buf, cmd);
  bg = parseline(buf, argv);
  
  if (argv[0] == NULL){return;} // Empty command line
  
  if (!built_in(argv)){
    // fork and get child process execve the executable file
    if ((pid = Fork()) == 0){
      if (execve(argv[0], argv, environ) < 0){
        perror("execve error");
        exit(0);
      }
    }
    // Register signal handler
    Signal(SIGINT, sigint_handler); 
    Signal(SIGCHILD, sigchild_handler);
    
    if (bg){
      printf("%d: %s", pid, cmd);
      jobs[cnt++] = pid;
    }else{
      fg_pid = pid;
      int stat;
      if (waitpid(pid, &stat, 0) < 0){
        perror("wait error");
      }
    }
  }
  return;
}

int main(){
  char cmd[MAXLEN];
  while (1){
    printf("$ ");
    fgets(cmd, MAXLEN, STDIN_FILENO);
    if (feof(STDIN_FILENO)){exit(0);} // detects EOF signal
    
    eval(cmd);
  }
  return 0;
}
