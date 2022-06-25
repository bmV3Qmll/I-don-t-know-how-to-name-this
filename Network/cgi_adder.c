#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAXRESP 2048

int main(){
    char * buf, resp[MAXRESP], * first, * sec;
    int n1, n2;
    if ((buf = getenv("QUERY_STRING")) != NULL){
        sprintf(resp, "QUERY_STRING=%s\r\n<p>", buf);
        strtok(buf, "=");
        first = strtok(NULL, "&");
        strtok(NULL, "=");
        sec = strtok(NULL, " \t\r\n");

        n1 = atoi(first);
        n2 = atoi(sec);
    }

    sprintf(resp, "%sThe answer is: %d + %d = %d\r\n<p>", resp, n1, n2, n1 + n2);
    sprintf(resp, "%sThanks for visiting!\r\n", resp);

    printf("Connection: close\r\n");
    printf("Content-type: text/html\r\n");
    printf("Content-length: %d\r\n\r\n", (int)strlen(resp));
    printf("%s", resp);
    fflush(stdout);
}
