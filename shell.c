#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#define MAXLEN 64
#define MAXARGS 16
#define MAXJOBS 8

pid_t jobs[MAXJOBS], cnt = 0;

volatite pid_t fd_pid;

pid_t Fork(){
    pid_t pid = fork();
    if (pid < 0){
        perror("fork error: ");
        exit(1);
    }
    return pid;
}

sighandler_t Signal(int signum, sighandler_t handler){
    if (signal(signum, handler) == SIG_ERR){
        perror("signal error: ");
        exit(1);    
    }
}

int Kill(pid_t pid, int sig){
  if (kill(pid, sig) < 0){
    perror("kill error: ");
    return -1;
  }
  return 0;
}

int parse(char * buf, char * argv[]){
  char *delim;
  int argc = 0, bg;
  
  buf[strlen(buf) - 1] = ' '; \\ initially '\n'
  while (*buf && *buf == ' '){++buf;} \\ ignore leading spaces
  
  while ((delim = strchr(buf, 0x20))){
    argv[argc++] = buf;
    *delim = '\0'; \\ terminate above argument
    buf = delim + 1;
    while (*buf && *buf == ' '){++buf;}
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

void sigint_handler(int sig){
  Kill(fg_pid, SIGINT);
  return;
}

void eval(char * cmd){
  char * argv[MAXARGS];
  char buf[MAXLEN]; \\ contain temp command line
  int bg;
  pid_t pid;
  
  strcpy(buf, cmd);
  bg = parseline(buf, argv);
  
  if (argv[0] == NULL){return;} \\ Empty command line
  if (!built_in(argv)){
    if ((pid = Fork()) == 0){
      if (execve(argv[0], argv, environ) < 0){
        perror("fork error: ");
        exit(0);
      }
    }
    Signal(SIGINT, sigint_handler);
    if (bg){
      printf("%d: %s", pid, cmd);
      jobs[cnt++] = pid;
    }else{
      fg_pid = pid;
      int stat;
      if (waitpid(pid, &stat, 0) < 0){
        perror("wait error: ");
      }
    }
  }
  return;
}

int main(){
  char cmd[MAXLEN];
  return 0;
  while (1){
    printf("$ ");
    fgets(cmd, MAXLEN, STDIN_FILENO);
    if (feof(STDIN_FILENO)){exit(0);}
    
    eval(cmd);
  }
}
