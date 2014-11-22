#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#define LISTENQ 5

int checkletter(const char*);
int checkunique(const int *, char (*)[64], int, int, const char*);
int checktell(const int *, char (*)[64], int, int, const char*);

int main(int argc, char* argv[]){
    struct sockaddr_in servaddr, cliaddr;
    int listenfd;

    if(argc != 2){
        printf("Usage: server <port>\n");
        return 0;
    }

    /* Set data of servaddr to 0 */
    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_family = PF_INET;
    servaddr.sin_port = htons(atoi(argv[1]));
    servaddr.sin_addr.s_addr = INADDR_ANY;

    /* Create a TCP socket */
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("ERROR: Create socket error\n");
        return 0;
    }

    /* bind the sockaddr with a socket */
    /* note that me must cast the sockaddr_in to sockaddr */
    if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        printf("ERROR: Bind error\n");
        return 0;
    }

    /* start listening */
    listen(listenfd, LISTENQ);

    int maxfd = listenfd; /* initialize */
    int maxi = -1; /* index into client[] array */
    fd_set rset, allset;
    int nready, client[FD_SETSIZE];
    char client_name[FD_SETSIZE][64]; /* To store client's name */
    int i,j;
    for(i=0; i<FD_SETSIZE; i++){
        client[i] = -1; /* -1 indicates available entry */
    }
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    /* main loop */
    while(1){
        int client_fd;
        int recv_len;
        char recv_buf[1024];
        char message[1024];
        socklen_t clilen = sizeof(cliaddr);

        rset = allset; /* structure assignment */
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);

        if(FD_ISSET(listenfd, &rset)){
            /* accept a client from the listening fd */
            client_fd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
            printf("A client connect in\n");

            for(i=0; i<FD_SETSIZE; i++){
                if(client[i]<0){
                    client[i] = client_fd; /* save descriptor */
                    snprintf(client_name[i], sizeof(client_name[i]), "anonymous");
                    break;
                }
            }
            if(i == FD_SETSIZE){
                printf("ERROR: Too many clients\n");
                close(client_fd);
                break;
            }

            FD_SET(client_fd, &allset); /* add new descriptor to set */
            if(client_fd > maxfd)
                maxfd = client_fd;
            if(i > maxi)
                maxi = i;
            
            /* When someone connect to the server */
            snprintf(message, sizeof(message), "[Server] Hello, %s! From: %s/%d\n", client_name[i], inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
            write(client_fd, message, strlen(message));
            snprintf(message, sizeof(message), "[Server] Someone is comming!\n");
            for(j=0; j<=maxi; j++){
                if(client[j]>=0 && j!=i){
                    write(client[j], message, strlen(message));
                }
            }


            if(--nready <= 0)
                continue;
        }
        
        int sock_fd;
        for(i=0; i<=maxi; i++){ /* check all clients for data */
            if((sock_fd = client[i]) < 0)
                continue;
            if(FD_ISSET(sock_fd, &rset)){
                /* read some message from the client */
                /* the read syscall returns 0 if the client close the connection */
                /* returns -1 if some error occurs */
                /* or returns the length of message we read */
                memset(recv_buf, 0, sizeof(recv_buf));

                recv_len = read(sock_fd, recv_buf, sizeof(recv_buf));
                if(recv_len < 0){
                    printf("ERROR: Read from client error\n");
                    close(sock_fd);
                }

                else if(recv_len == 0){ /* someone offline */
                    printf("The client close the connection\n");
                    close(sock_fd);
                    FD_CLR(sock_fd, &allset);
                    client[i] = -1;
                    for(j=0; j<=maxi; j++){
                        if(client[j] >= 0){
                            snprintf(message, sizeof(message), "[Server] %s is offline\n", client_name[i]);
                            write(client[j], message, strlen(message));
                        }
                    }
                }
                else { /* client send message to server */
                    printf("Receive message from the client, message is %s\n", recv_buf);
                    char *token[128], *p;
                    for(j=0, p=strtok(recv_buf, " \t\n"); p!=NULL; p=strtok(NULL, " \t\n")){
                        token[j++] = p;
                    }
                    if(j==0 && p==NULL)
                        if(--nready <= 0) /* no more readable descriptors */
                            break;
                        else
                            continue;

                    if(strcmp(token[0], "who") == 0){  /* call who */
                        for(j=0; j<=maxi; j++){
                            getsockname(client[j], (struct sockaddr*)&cliaddr, &clilen);
                            if(client[j] >= 0 && j!=i){
                                snprintf(message, sizeof(message), "[Server] %s %s/%d\n", client_name[j], inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
                            }
                            else if(client[j] >= 0 && j==i){
                                snprintf(message, sizeof(message), "[Server] %s %s/%d -> me\n", client_name[j], inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
                            }
                            write(client[i], message, strlen(message));
                        }
                    }
                    else if(strcmp(token[0], "name") == 0){ /* call rename */
                        if(strcmp(token[1], "anonymous") == 0){ /* name is anonymous */
                            snprintf(message, sizeof(message), "[Server] ERROR: Username cannot be anonymous.\n");
                        }
                        else if(checkletter(token[1]) == 0){ /* name format error */
                            snprintf(message, sizeof(message), "[Server] ERROR: Username can only consists of 2~12 English letters.\n");
                        }
                        else if(checkunique(client, client_name, i, maxi, token[1]) == 0){ /* name is not unique */
                            snprintf(message, sizeof(message), "[Server] ERROR: %s has been used by others.\n", token[1]);
                        }
                        else { /* change name success */
                            snprintf(message, sizeof(message), "[Server] You're now known as %s.\n", token[1]);
                            char broadcast[200];
                            memset(broadcast, 0, sizeof(broadcast));
                            snprintf(broadcast, sizeof(broadcast), "[Server] %s is now known as %s.\n", client_name[i], token[1]);
                            snprintf(client_name[i], sizeof(client_name[i]), "%s", token[1]);
                            for(j=0; j<=maxi; j++){
                                if(client[j] >= 0)
                                    if(i!=j)
                                        write(client[j], broadcast, strlen(broadcast));
                            }
                        }
                        write(sock_fd, message, strlen(message));
                    }
                    else if(strcmp(token[0], "tell") == 0){ /* private message */
                        int result = checktell(client, client_name, i, maxi, token[1]);
                        if(result == -1){
                            snprintf(message, sizeof(message), "[Server] ERROR: You are anonymous.\n");
                        }
                        else if(result == -2){
                            snprintf(message, sizeof(message), "[Server] ERROR: The client to which you sent is anonymous.\n");
                        }
                        else if(result == -3){
                            snprintf(message, sizeof(message), "[Server] ERROR: The receiver doesn't exist.\n");
                        }
                        else{
                            snprintf(message, sizeof(message), "[Server] SUCCESS: Your message has been sent.\n");
                            char send_message[1024];
                            memset(&send_message, 0, sizeof(send_message));
                            snprintf(send_message, sizeof(send_message), "%s tell you %s\n", client_name[i], token[2]);
                            write(client[result], send_message, strlen(send_message));
                        }
                        write(sock_fd, message, strlen(message));
                    }
                    else if(strcmp(token[0], "yell") == 0){ /* broadcast message */
                        snprintf(message, sizeof(message), "[Server] %s yell %s\n", client_name[i], token[1]);
                        for(j=0; j<=maxi; j++){
                            if(client[j] >= 0){
                                write(client[j], message, strlen(message));
                            }
                        }
                    }
                    else { /* command not found */
                        snprintf(message, sizeof(message), "[Server] ERROR: Error command.\n");
                        write(sock_fd, message, strlen(message));
                    }
                }

                if(--nready <= 0){ /* no more readable descriptors */
                    break;
                }
            }
        }
    }
}

int checkletter(const char* str){
    if((strlen(str)>12) || (strlen(str)<2))
        return 0;
    int i;
    for(i=0; i<strlen(str); i++){
        if(!((str[i]<='Z')&&(str[i]>='A') || (str[i]<='z')&&(str[i]>='a')))
            return 0;
    }

    return 1;
}

int checkunique(const int *client, char (*client_name)[64], int i, int maxi, const char* name){
    int j;
    for(j=0; j<=maxi; j++){
        if(client[j] >= 0 && j!=i){
            if(strcmp(name, client_name[j]) == 0)
                return 0;
        }
    }

    return 1;
}

int checktell(const int *client, char (*client_name)[64], int i, int maxi, const char* receiver){
    if(strcmp(client_name[i], "anonymous") == 0)
        return -1;
    if(strcmp(receiver, "anonymous") == 0)
        return -2;

    int j;
    for(j=0; j<=maxi; j++){
        if(client[j] >= 0)
            if(strcmp(receiver, client_name[j]) == 0)
                return j;
                
    }
    return -3;
}
