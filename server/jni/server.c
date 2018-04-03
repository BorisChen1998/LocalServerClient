/* A simple server in the internet domain using TCP
   The port number is passed as an argument
   This version runs forever, forking off a separate
   process for each connection
*/
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


int dostuff(int, int *); /* function prototype */
void error(const char *msg) {
  perror(msg);
  exit(1);
}
int sum = 0;
int has_add_in_shared = 0;

int main(int argc, char *argv[]) {
  
  int * shared = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0); 
  int sockfd, newsockfd, portno, status;
  pid_t pid1, pid2, w1, w2;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  //signal(SIGCHLD,signal_catchmy);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");
  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = 2050;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");
  while (1) {
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
      error("ERROR on accept");

    pid1 = fork();
    if (pid1 < 0)
      error("ERROR on fork");
    if (pid1 == 0) {
      close(sockfd);
      while (dostuff(newsockfd, shared));
      (*shared)--;
      exit(0);
    } else
      close(newsockfd);
  } /* end of while */
  close(sockfd);
  return 0; /* we never get here */
}

/******** DOSTUFF() *********************
 There is a separate instance of this function
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
int dostuff(int sock, int * shared) {
  int n;
  char buffer[256];
  char msg[3];
  bzero(buffer, 256);
  n = read(sock, buffer, 255);
  if (n < 0)
    error("ERROR reading from socket");

  if (buffer[0] == ':' && buffer[1] == 'q' && buffer[2] == '\n') {
    if (has_add_in_shared == 0) {
      (*shared)++;
      has_add_in_shared = 0;
    }

    printf("Server thread closing...\n");

    n = write(sock, "Client closing...\n", 18);
    if (n < 0)
      error("ERROR writing to socket");
    
    return 0;
  } else if (*shared == 2) {
    strcpy(buffer, "Please wait...");
    n = write(sock, buffer, 14);
    if (n < 0)
      error("ERROR writing to socket");
    return 1;
  } 
  else {
    if (has_add_in_shared == 0) {
      (*shared)++;
      has_add_in_shared = 1;
    }

    printf("Receiving message: %s", buffer);
    int tmp = 0;
    while (1) {
      if (buffer[tmp] == 0)
        break;
      if (buffer[tmp] >= 'a' && buffer[tmp] <= 'z') buffer[tmp] = (buffer[tmp]-'a'+3)%26+'a';
      else if (buffer[tmp] >= 'A' && buffer[tmp] <= 'Z') buffer[tmp] = (buffer[tmp]-'A'+3)%26+'A';
      tmp++;
    }
    n = write(sock, buffer, tmp);
    if (n < 0)
      error("ERROR writing to socket");

    return 1;
  }
}
