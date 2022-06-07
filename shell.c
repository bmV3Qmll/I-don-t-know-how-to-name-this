#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

#define MAXLEN 64
#define MAXARGS 16

pid_t Fork(){
    pid_t pid = fork();
    if (pid < 0){
        perror("fork error: ");
        exit(1);
    }
    return pid;
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
      
      break;
    case "bg":
      
      break;
    case "fg":
      
      break;
    default:
      return 0;
  }
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
    if (bg){
      
    }else{
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
