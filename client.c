#include <netdb.h>
#include <signal.h>
#include <stdio.h> 
#include <string.h>   
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h>   
#include <arpa/inet.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h>

#define MAX 128
// #define PORT 49000 
#define SA struct sockaddr
#define max_room_pop 5

int times_up;
int sockfd, sockfdToCli;
struct sockaddr_in servaddr, cliaddr;

void sig_handler(int signum){ 
  times_up = 1;
}

void func()
{
    char buff[MAX];
    char msg[MAX];
    int n;
    int bidding_mode = 0;
    int project_id = -1;
    int client_id;
    int bid;
    int end_bid_counter;
    int my_turn;
    int broadcast_project;
    int broadcast_bid;
    int turn;
    int lowest_bid;
    int bid_sent;

    bzero(buff, sizeof(buff));
    read(sockfd, buff, sizeof(buff));
    printf("From Server : %s", buff);
    read(sockfd, &client_id, sizeof(int));
    printf("Client ID is : %d\n", client_id);
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
            if((strncmp(buff, "exit", 4)) == 0){
                printf("exitting bid1.00 ...\n");
                break;
            }
            project_id = atoi(buff);
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
                if ((strncmp(buff, "Bidding has commenced", 21)) == 0){
                    bidding_mode = 1;
                    lowest_bid = -1;
                    end_bid_counter = 0;
                    broadcast_project = project_id;
                    broadcast_bid = -1;
                    // printf("Waiting for recv 1\n");
                    recv(sockfd, &turn, sizeof(int), 0);
                    // printf("Waiting for recv 2\n");
                    recv(sockfd, &my_turn, sizeof(int), 0);
                    printf("going to bid...\n");
                }
            }
        }
        else if (bidding_mode == 1){
            if(project_id == broadcast_project){
                printf("awaiting your turn to bid...\n");
                printf("%s\n", buff);
                if((strncmp(buff, "Bidding has ended", 17)) == 0){
                    bidding_mode = 0;
                    write(sockfd, buff, sizeof(buff));
                    continue;
                }
                if (broadcast_bid > 0 && (broadcast_bid < lowest_bid || lowest_bid < 1)){
                        lowest_bid = broadcast_bid;
                        end_bid_counter = 1;
                }
                else if (end_bid_counter > 0) end_bid_counter++;
                if (lowest_bid > 0)
                    printf("current lowest bid is %d\n", lowest_bid);
                else
                    printf("current lowest bid is None\n");

                if ( turn != my_turn )
                {
                    printf("it is client %d's turn ----- lowest_bid is %d\n", turn, lowest_bid);
                }
                else
                {
                    // printf("hello! im bidding --- lowest_bid is %d\n", lowest_bid);
                   bid_sent = 0;
                   times_up = 0;
                   alarm(10);
                   printf("Enter your bid : ");
                   bzero(buff, sizeof(buff));
                   n = 0;
                   scanf("%s", buff);
                   printf("bid pending...\n");
                   if (times_up == 0){
                        alarm(0);
                        bid_sent = 1;
                        bid = atoi(buff);
                        if (bid > 0 && (bid < lowest_bid || lowest_bid < 1))
                        {
                            end_bid_counter = 1;
                            lowest_bid = bid;
                            sprintf(msg, "Client %d has bidded new minimum bid : %d\r\n", client_id, bid);
                        }
                        else{
                            if (end_bid_counter > 0) end_bid_counter++;
                            sprintf(msg, "Client %d's bid : %s\r\n", client_id,buff);
                        }
                        
                        turn++;
                        printf("your bid is sent.\n");
                   }
                   else{
                    if (!bid_sent)
                    {
                        if (end_bid_counter > 0) end_bid_counter++;
                        bid = 0;
                        printf("Times up!\n");
                        sprintf(msg, "Client %d was unable to bid in time\r\n", client_id);
                        turn++;
                    }
                   }
                   turn = turn % max_room_pop;
                   if(end_bid_counter >= max_room_pop)
                        strcpy(msg, "Bidding has ended\r\n");
                   sendto(sockfdToCli,&project_id,sizeof(int), 0,(struct sockaddr *)&cliaddr, sizeof cliaddr);
                    sendto(sockfdToCli,msg,MAX, 0,(struct sockaddr *)&cliaddr, sizeof cliaddr);
                    sendto(sockfdToCli,&turn,sizeof(int), 0,(struct sockaddr *)&cliaddr, sizeof cliaddr);
                    sendto(sockfdToCli,&bid,sizeof(int), 0,(struct sockaddr *)&cliaddr, sizeof cliaddr);
                }
            
            }
            bzero(buff, sizeof(buff));
            // printf("waiting for recv broadcast_project\n");
            recv(sockfdToCli, &broadcast_project, sizeof(int), 0);
            // printf("waiting for recv buff\n");
            recv(sockfdToCli, buff, sizeof(buff), 0);
            // printf("waiting for recv turn\n");
            recv(sockfdToCli, &turn, sizeof(turn), 0);
            // printf("waiting for recv broadcast_bid\n");
            recv(sockfdToCli, &broadcast_bid, sizeof(int), 0);
            // printf("recv complete\n");

        }
    }
}
  
int main(int argc , char *argv[])
{
    int PORT = atoi(argv[1]);
    int connfd;
    
    signal(SIGALRM,sig_handler);
    siginterrupt(SIGALRM,1);

    int broadcast = 1;
    int optim = 1;

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

    sockfdToCli = socket(AF_INET,SOCK_DGRAM,0);
    setsockopt(sockfdToCli,SOL_SOCKET,SO_BROADCAST,&broadcast,sizeof(broadcast));
    setsockopt(sockfdToCli,SOL_SOCKET,SO_REUSEPORT,&optim,sizeof(optim));

    cliaddr.sin_family = AF_INET;     
    cliaddr.sin_port = htons(atoi(argv[1])); 
    cliaddr.sin_addr.s_addr = inet_addr("255.255.255.255");
    bind(sockfdToCli,(struct sockaddr *)&cliaddr,sizeof cliaddr);
  
    func();
  
    close(sockfd);
}