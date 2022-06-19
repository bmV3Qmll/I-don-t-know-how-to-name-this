#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "robust_IO.h" 
#include "socket_wrapper.h"

#define MAXLEN 1024
#define MAXRESP 2048

extern char **environ;

pid_t Fork(){
    pid_t pid = fork();
    if (pid < 0){
        perror("fork error");
        exit(1);
    }
    return pid;
}

void error(int fd, char * cause, int code, char * msg, char * desc){
    char buf[MAXLEN], body[MAXRESP];

    sprintf(body, "<html><title>Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%d: %s\r\n", body, code, msg);
    sprintf(body, "%s<p>%s: %s\r\n", body, desc, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
    
    sprintf(buf, "HTTP/1.0 %d %s\r\n", code, msg);
    writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    writen(fd, buf, strlen(buf));
    writen(fd, body, strlen(body));
}

void process_reqhdrs(struct rio * rp){
    char buf[MAXLEN];

    while(1){
        buf_readline(rp, buf, MAXLEN);
        if (strcmp(buf, "\r\n")){break;}
        printf("%s", buf);
    }
}

int parse_uri(char * uri, char * file, char * cgiargs){
    strcpy(file, "/var/www/html");  // '.' indicates the home directory for webserver, can be replaced
    strcat(file, uri);

    if (strstr(uri, "/cgi-bin/") == NULL){
        if (uri[strlen(uri)-1] == '/'){
            strcat(file, "index.html");
        }
        strcpy(cgiargs, "");
        return 1;
    }
    
    char * delim;
    if ((delim = strchr(file, '?'))){
        *delim = '\0';
        strcpy(cgiargs, delim + 1);
    }else{strcpy(cgiargs, "");}
    return 0;
}

void query_static(int fd, char * filename, int filesize){
    int srcfd;
    char resp[MAXRESP], filetype[16], * filecopy;

    // determine filetype by suffix
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else
        strcpy(filetype, "text/plain");

    srcfd = open(filename, O_RDONLY, 0);
    if (srcfd == -1){
        error(fd, filename, 500, "Internal Server Error", "Unable to open");
        return;
    }
    
    if ((filecopy = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0)) == (void *) -1){
        error(fd, filename, 500, "Internal Server Error", "Unable to copy");
        return;
    }

    // Process response headers
    sprintf(resp, "HTTP/1.0 200 OK\r\n");
    sprintf(resp, "%sServer: Tiny Web Server\r\n", resp);
    sprintf(resp, "%sConnection: close\r\n", resp);
    sprintf(resp, "%sContent-length: %d\r\n", resp, filesize);
    sprintf(resp, "%sContent-type: %s\r\n\r\n", resp, filetype);
    writen(fd, resp, strlen(resp));
    printf("\nResponse:\n%s", resp);

    // Send response body
    writen(fd, filecopy, filesize);

    // Clean up
    close(srcfd);
    munmap(filecopy, filesize);
}

void query_dynamic(int fd, char * filename, char * cgiargs){
    char resp[MAXRESP], * empty_argv[] = {NULL};
    int pid;

    sprintf(resp, "HTTP/1.0 200 OK\r\n");
    sprintf(resp, "%sServer: Tiny Web Server\r\n", resp);
    writen(fd, resp, strlen(resp));

    if ((pid = Fork()) == 0){
        dup2(fd, STDOUT_FILENO);    // redirect child's output to client
        setenv("QUERY_STRING", cgiargs, 1); // pull arguments to cgi program

        if (execve(filename, empty_argv, environ) == -1){
            perror("execve error");
            exit(1);
        }
    }

    if (waitpid(pid, NULL, 0) == -1){
        perror("waitpid error");
        return;
    }
}

void serve(int connfd){
    char method[16], uri[64], version[16], filename[64];
    char buf[MAXLEN], arg[MAXLEN];
    struct rio rp;
    struct stat st;
    
    rio_init(&rp, connfd);
    buf_readline(&rp, buf, MAXLEN);
    printf("Request:\n%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, (char *) "GET")){
        error(connfd, method, 501, "Not implemented", "Server doesn't support this method :(");
        	printf("error ok\n");
	return;
    }

    process_reqhdrs(&rp);

    int is_static = parse_uri(uri, filename, arg);

    if (stat(filename, &st) == -1){
        error(connfd, filename, 404, "Not found", "File not found");
	return;
    }

    if (is_static){
        // check if filename is a regular, readable file
        if (!S_ISREG(st.st_mode) || !(st.st_mode & S_IRUSR)){
            error(connfd, filename, 403, "Forbidden", "Low privilege");
	    return;
        }

        query_static(connfd, filename, st.st_size);
    }else{
        // check if filename is a regular, executable file
        if (!S_ISREG(st.st_mode) || !(st.st_mode & S_IXUSR)){
            error(connfd, filename, 403, "Forbidden", "Low privilege");
	    return;
	}

        query_dynamic(connfd, filename, arg);
    }
}

int main(int argc, char * argv[]){
    if (argc != 2){
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}
	
	int listenfd, connfd;
	if ((listenfd = open_listenfd(argv[1])) == -1){
		fprintf(stderr, "unable to open port %s", argv[1]);
		exit(1);
	}
	
	struct sockaddr * client;
	socklen_t addrlen;
	char host[MAXLEN], port[MAXLEN];
	
	while(1){
		addrlen = sizeof(struct sockaddr);
		connfd = accept(listenfd, client, &addrlen);
		if (getnameinfo(client, addrlen, host, MAXLEN, port, MAXLEN, 0) == 0){
			printf("Receive connection from %s:%s\n", host, port);
			serve(connfd);
		}
		close(connfd);
	}
	
    return 0;
}
