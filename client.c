#include <netdb.h>
#include <signal.h>
#include <stdio.h> 
#include <string.h>   //strlen 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>   //close 
#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h>

#define MAX 80
// #define PORT 49000 
#define SA struct sockaddr


int times_up;
int bid_sent = 0;
int sockfd;

void sig_handler(int signum){ 
  times_up = 1;
  char no_bid[] = "no_bid\r\n";
  if (!bid_sent)
   {
      printf("Times up! Press any key + enter to continue");
      write(sockfd, no_bid, sizeof(no_bid));
    }
}

void func()
{
    char buff[MAX];
    int n;
    int bidding_mode = 0;
    int project_id = -1;

    bzero(buff, sizeof(buff));
    read(sockfd, buff, sizeof(buff));
    printf("From Server : %s", buff);
    for (;;) {
        if (bidding_mode == 0){
            printf("Following projects are available:\n");
            bzero(buff, sizeof(buff));
            read(sockfd, buff, sizeof(buff));
            printf("%s\n", buff);
            printf("Enter project number : ");
            bzero(buff, sizeof(buff));
            n = 0;
            scanf("%s", buff);
            // while ((buff[n++] = getchar()) != '\n');
            project_id = atoi(buff);
            // buff[n-1] = '\r';
            // buff[n] = '\n';
            // buff[n+1] = '\0';
            printf("requesting project %d bidding room from server...\n", project_id);
            write(sockfd, buff, sizeof(buff));
            bzero(buff, sizeof(buff));
            read(sockfd, buff, sizeof(buff));
            printf("From Server : %s", buff);
            if ((strncmp(buff, "Request accepted", 16)) == 0){
                printf("Waiting for room to be ready...\n");
                bzero(buff, sizeof(buff));
                read(sockfd, buff, sizeof(buff));
                printf("From Server : %s", buff);
                // printf("hereee\n");
                if ((strncmp(buff, "Bidding has commenced", 21)) == 0){
                    bidding_mode = 1;
                    printf("going to bid...\n");
                    write(sockfd, "send list", sizeof("send list"));
                }
            }
        }
        else if (bidding_mode == 1){
            printf("awaiting your turn to bid...\n");
            bzero(buff, sizeof(buff));
            read(sockfd, buff, sizeof(buff));
            printf("From Server : %s", buff);
            if ((strncmp(buff, "Your turn", 9)) == 0){
               bid_sent = 0;
               times_up = 0;
               alarm(10);
               printf("Enter your bid : ");
               bzero(buff, sizeof(buff));
               n = 0;
               scanf("%s", buff);
               // while ((buff[n++] = getchar()) != '\n' && times_up == 0);
               // buff[n-1] = '\r';
               // buff[n] = '\n';
               // buff[n+1] = '\0';
               printf("bid pending...\n");
               if (times_up == 0){
                bid_sent = 1;
                write(sockfd, buff, sizeof(buff));
                printf("your bid is sent.\n");
                alarm(0);
               }
               else{
               }
            }
            if ((strncmp(buff, "Bidding has ended", 17)) == 0){
                bidding_mode = 0;
                write(sockfd, buff, sizeof(buff));
            }
        }
    }
}
  
int main(int argc , char *argv[])
{
    int PORT = atoi(argv[1]);
    int connfd;
    struct sockaddr_in servaddr, cli;
    
    signal(SIGALRM,sig_handler);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
  
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);
    
    // fcntl(sockfd, F_SETFL, SOCK_NONBLOCK);
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("connection with the server failed...\n");
        exit(0);
    }
    else
        printf("connected to the server..\n");
  
    func();
  
    close(sockfd);
}