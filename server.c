#include <stdio.h> 
#include <signal.h>
#include <string.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/time.h>
     
#define TRUE   1 
#define FALSE  0 
// #define PORT 49000 
#define max_clients 3
#define max_room_pop 3

enum CLIENT_STATE{IDLE, REQ_PROJ, JOINED_ROOM, WAITING_TO_BID, BIDDING};

void switch_turn(int proj, int num_of_projects, int* projects_room_client_turn, int* client_socket, int* projects_room_min_bid, int* client_req_proj){
  char msg[64];
  // for (int i =0; i < max_clients; i++){
        int i = projects_room_client_turn[proj];
        int next_i = (i+1)%max_clients;
        for (int j = 0; j < max_clients; j++){
            next_i = (i+1+j)%max_clients;
            if ( client_req_proj[next_i] == proj && next_i != i)
                break;
        }
        projects_room_client_turn[proj] = next_i;
        printf("client %d's turn\n", next_i );
        sprintf(msg, "Your turn : current lowest bid is %d\r\n", projects_room_min_bid[proj]);
        // sprintf()
        // send();
        if (send(client_socket[next_i], msg, strlen(msg), 0) != strlen(msg) )
            perror("send");                 
    // }
  // }
}

void sig_handler(int signum){ 
  // times_up = 1;
  // char *your_turn_msg = "Your turn \r\n";
  // switch_turn();
  alarm(10);
}

int main(int argc , char **argv)  
{  
    int num_of_projects = 4;
    int projects_room_client_turn[] = {0, 0, 0, 0};
    int client_socket[max_clients];
    int projects_room_min_bid[] = {1000000, 1000000, 1000000, 1000000};
    int client_req_proj[max_clients];


    int projects_end_bid_counter[] = {0,0,0,0};
    int bidding_mode = 0;
    printf("%s\n", argv[1]);
    int PORT = atoi(argv[1]);
    int opt = TRUE;  
    int master_socket , addrlen , new_socket , activity, i , valread , sd;  
    // int max_clients = 3 ,
    // int client_req_proj[max_clients];
    int client_state[max_clients];
    for (int i = 0; i < max_clients; i++){
        client_req_proj[i] = -1;
        client_state[i] = IDLE;
    }

    int max_sd;  
    struct sockaddr_in address;  

    int projects[] = {1,2,3,4};
    int projects_room_pop[] = {0,0,0,0};
    long bid;
    int projects_room_min_bid_client[] = {-1, -1, -1 ,-1};
    // int projects_room_client_turn[] = {0, 0, 0, 0};
    int projects_avail[] = {1, 1, 1, 1};

    char *list_of_projects = malloc(256);
    bzero(list_of_projects, sizeof(list_of_projects));
         
    char buffer[1025];
         
    fd_set readfds;  
         
    char welcome_message[64];
    char *start_bid_message = "Bidding has commenced \r\n";
    char *end_bid_message = "Bidding has ended ";
    char *req_acc_msg = "Request accepted \r\n";
    char *req_dny_msg = "Request denied \r\n";
    char *room_ready_msg = "Room is ready \r\n";

    char my_str[64];

    signal(SIGALRM,sig_handler);

    for (i = 0; i < max_clients; i++)  
    {  
        client_socket[i] = 0;  
    }  
         
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)  
    {  
        perror("socket failed");  
        exit(EXIT_FAILURE);  
    }  
     
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, 
          sizeof(opt)) < 0 )  
    {  
        perror("setsockopt");  
        exit(EXIT_FAILURE);  
    }  
     
    address.sin_family = AF_INET;  
    address.sin_addr.s_addr = INADDR_ANY;  
    address.sin_port = htons( PORT );  
         
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0)  
    {  
        perror("bind failed");  
        exit(EXIT_FAILURE);  
    }  
    printf("Listener on port %d \n", PORT);  
         
    if (listen(master_socket, max_clients) < 0)  
    {  
        perror("listen");  
        exit(EXIT_FAILURE);  
    }  
         
    //accept the incoming connection 
    addrlen = sizeof(address);  
    puts("Waiting for connections ...");  
    int num_of_clients = 0;
         
    while(1)  
    {  
        FD_ZERO(&readfds);  
     
        FD_SET(master_socket, &readfds);  
        max_sd = master_socket;  
             
        for ( i = 0 ; i < max_clients ; i++)  
        {  
            sd = client_socket[i];  
                 
            if(sd > 0)  
                FD_SET( sd , &readfds);  
                 
            if(sd > max_sd)  
                max_sd = sd;  
        }  
     
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);  
       
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select error");  
        }  
             
        if (FD_ISSET(master_socket, &readfds))  
        {  
            if ((new_socket = accept(master_socket, 
                    (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)  
            {  
                perror("accept");  
                exit(EXIT_FAILURE);  
            }  
             
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
           
            for (i = 0; i < max_clients; i++)  
            {  
                if( client_socket[i] == 0 )  
                {  
                    num_of_clients += 1;
                    client_req_proj[i] = -1;
                    client_state[i] = REQ_PROJ;
                    client_socket[i] = new_socket;  
                    printf("Adding to list of sockets as %d\n" , i);  
                    printf("Number of clients is now %d\n" , num_of_clients);  
                    
                    sprintf(welcome_message, "Welcome to bid1.00!\n Your Client ID is %d\r\n" , i);
                    if( send(new_socket, welcome_message, strlen(welcome_message), 0) != strlen(welcome_message) )  {  
                        perror("send");  
                    } 
                    else puts("Welcome message sent successfully");  

                    break;  
                }  
            }

            //send list of projects
            bzero(list_of_projects, sizeof(list_of_projects));
            strcpy(list_of_projects, "");
            for (i = 0; i < num_of_projects; i++){
                if(projects_avail[i] == 1){
                    sprintf(my_str, "Project %d\n", projects[i]);
                    strcat(list_of_projects, my_str);
                }
            }
            strcat(list_of_projects, "\r\n");
            if( send(new_socket, list_of_projects, strlen(list_of_projects), 0) != strlen(list_of_projects) )  
            {  
                perror("send");  
            }  
        }  
             
        for (i = 0; i < max_clients; i++)  
        {  
            sd = client_socket[i];  
                 
            if (FD_ISSET( sd , &readfds))  
            {  
                valread = read( sd , buffer, 1024);
                if ( strcmp(buffer,"Exit") == 0 )  
                {  
                    getpeername(sd , (struct sockaddr*)&address , \
                        (socklen_t*)&addrlen);  
                    printf("Host disconnected , ip %s , port %d \n" , 
                          inet_ntoa(address.sin_addr) , ntohs(address.sin_port));  
                         
                    num_of_clients -= 1;
                    printf("Number of clients is now %d\n" , num_of_clients);  

                    close( sd );  
                    client_socket[i] = 0;  
                    client_state[i] = IDLE;
                    client_req_proj[i] = -1;
                }  
                     
                else  
                {   
                    printf("from client %d : %s\n", i, buffer);
                    if (client_state[i] == REQ_PROJ){
                        int requested_proj = atoi(buffer) - 1;
                        printf("client %d has requested project %d\n", i, projects[requested_proj]);
                        
                        if (projects_room_pop[requested_proj] < max_room_pop && projects_avail[requested_proj]){
                            printf("client %d is in room project %d\n", i, projects[requested_proj]);
                            client_req_proj[i] = requested_proj;
                            projects_room_pop[requested_proj] += 1;
                            if (send(client_socket[i], req_acc_msg, strlen(req_acc_msg), 0) != strlen(req_acc_msg) )
                                perror("send");                 
                            
                            if (projects_room_pop[requested_proj] == max_room_pop){
                                printf("begin bidding for project %d \n", requested_proj+1);
                                projects_end_bid_counter[requested_proj] = 0;
                                for(int j=max_clients-1; j >= 0; j--){
                                // for(int j =0; j <max_clients; j++){
                                    if (client_req_proj[j] == requested_proj){
                                        client_state[j] = BIDDING;
                                        
                                        if (send(client_socket[j], start_bid_message, strlen(start_bid_message), 0) != strlen(start_bid_message) )
                                            perror("send");   
                                        projects_room_client_turn[requested_proj] = j;
                                        // alarm(10);
                                    }
                                }
                                int j = projects_room_client_turn[requested_proj];
                                char msg[64];
                                printf("client %d's turn\n", j );
                                sprintf(msg, "Your turn : (current lowest bid is %d)\r\n", projects_room_min_bid[requested_proj]);
                                // sprintf()
                                // send();
                                if (send(client_socket[j], msg, strlen(msg), 0) != strlen(msg) )
                                    perror("send");                 
                                break;
                            }
                        }


                    }
                    else if (client_state[i] == BIDDING){
                        int proj = client_req_proj[i];
                        printf( "projects_room_client_turn[%d] = %d and i = %d\n", proj,projects_room_client_turn[proj], i);
                        if (projects_room_client_turn[proj] == i){
                            bid = atoi(buffer);
                            projects_end_bid_counter[proj] += 1;
                            if (bid > 0){
                                if( projects_room_min_bid[proj] > bid )
                                    projects_end_bid_counter[proj] = 1;
                                    printf("new lowest bid for project %d: %ld for client %d\n", proj+1, bid, i);
                                    projects_room_min_bid[proj] = bid;
                                    projects_room_min_bid_client[proj] = i;
                            }
                            switch_turn(proj,num_of_projects, projects_room_client_turn, client_socket, projects_room_min_bid, client_req_proj);
                        }
                        else{
                            printf("invalid bid\n");
                            if (send(client_socket[i], req_dny_msg, strlen(req_dny_msg), 0) != strlen(req_dny_msg) )
                                perror("send");                 
                            // switch_turn(proj);
                        }

                        if ( projects_end_bid_counter[proj] == max_room_pop){
                            char msg[64];
                            printf("Bidding for project %d has ended\n", proj);
                            printf("Client %d won the project. Congaratulations!", projects_room_min_bid_client[proj]);
                            for (int j = 0; j < max_clients; j++){
                                if (client_req_proj[j] == proj){
                                    strcpy(msg, end_bid_message);
                                    if (j == projects_room_min_bid_client[proj])
                                        strcat(msg, "You won the project.\r\n");
                                    else
                                        strcat(msg, "You were outbidded.\r\n");
                                    if (send(client_socket[j], msg, strlen(msg), 0) != strlen(msg) )
                                        perror("send");                 
                                    client_req_proj[j] = -1;
                                    client_state[j] = IDLE;
                                }
                            }
                            projects_avail[proj] = 0;
                        }
                    }
                    else{
                        bzero(list_of_projects, sizeof(list_of_projects));
                        strcpy(list_of_projects, "");

                        for (i = 0; i < num_of_projects; i++){
                            if(projects_avail[i] == 1){
                                sprintf(my_str, "Project %d\n", projects[i]);
                                strcat(list_of_projects, my_str);
                            }
                        }
                        strcat(list_of_projects, "\r\n");
                        if( send(new_socket, list_of_projects, strlen(list_of_projects), 0) != strlen(list_of_projects) )  
                        {  
                            perror("send");  
                        }  
                    }
                }  
            }  
        }  
    }  
         
    return 0;  
}  

