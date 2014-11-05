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

int cal(int client_fd, char* buf, int *AC, char *send);

int main(int argc, char* argv[]){
    int i,j,maxi,maxfd;
    int nready, client[FD_SETSIZE];
    fd_set rset, allset;

    struct sockaddr_in servaddr, cliaddr;
    int listenfd;

    if(argc != 2){
        printf("Usage: server <port>\n");
        return 0;
    }

    /* Set data of servaddr to 0 */
    memset(&servaddr, 0, sizeof(servaddr)); 
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
    listen(listenfd, 5);

    /* initialization */
    maxfd = listenfd;
    maxi = -1;
    for(i=0; i<FD_SETSIZE; i++){
        client[i] = -1;
    }
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    /* main loop */
    while(1){
        int client_fd;
        int recv_len;
        char recv_buf[1024];
        char send_buf[1024];
        int AC[FD_SETSIZE];
        socklen_t clilen = sizeof(cliaddr);
        rset = allset;
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);

        if(FD_ISSET(listenfd, &rset)){
            /* accept a client from the listening fd */
            client_fd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
            printf("A client connect in\n");

            for(i=0; i<FD_SETSIZE; i++){
                if(client[i]<0){
                    client[i] = client_fd; /* save descriptor */
                    AC[i] = 0;
                    break;
                }
            }
            if(i == FD_SETSIZE){
                printf("Too much clients\n");
                close(client_fd);
                break;
            }
            FD_SET(client_fd, &allset);
            if(client_fd > maxfd)
                maxfd = client_fd;
            if(i > maxi)
                maxi = i;

            for(j=0; j<=maxi; j++){
                if(client[j]>=0){
                    printf("client[%d] = %d\n", j, client[j]);
                }
            }

            if(--nready <= 0)
                continue;
        }

        for(i=0; i<=maxi; i++){
            if(client[i] < 0)
                continue;
            if(FD_ISSET(client[i], &rset)){
                printf("Now dealing with client[%d] = %d\n", i, client[i]);
                /* read some message from the client */
                memset(recv_buf, 0, sizeof(recv_buf));
                memset(send_buf, 0, sizeof(send_buf));

                recv_len = read(client[i], recv_buf, sizeof(recv_buf));
                printf("Read finished\n");
                if(recv_len < 0){
                    printf("ERROR: Read from client error\n");
                    close(client[i]);
                    FD_CLR(client[i], &allset);
                    client[i] = -1;
                    break;
                }

                else if(recv_len == 0){
                    printf("The client close the connection\n");
                    close(client[i]);
                    FD_CLR(client[i], &allset);
                    client[i] = -1;
                    break;
                }
                else {
                    printf("Receive message from the client, message is %s\n", recv_buf);
                    int result = cal(client[i], recv_buf, (AC+i), send_buf);
                    if(result == 0){
                        FD_CLR(client[i], &allset);
                        client[i] = -1;
                        break;
                    }
                }
                printf("Now complete dealing with client[%d] = %d\n", i, client[i]);
                if(--nready <=0)
                    break;
            }
        }
    }
}

int cal(int client_fd, char *buf, int *AC, char *send){
    char *token[10], *p;
    int i;
    for(i=0, p=strtok(buf, " \n\t"); p!=NULL; p=strtok(NULL, " \n\t")){
        token[i++] = p;
    }

    if(i==0 && token[0]==NULL)
        return 1;
    if(strcmp(token[0], "SET") == 0){
        *AC = atoi(token[1]);
    }
    else if(strcmp(token[0], "ADD") == 0){
        *AC += atoi(token[1]);
    }
    else if(strcmp(token[0], "SUB") == 0){
        *AC -= atoi(token[1]);
    }
    else if(strcmp(token[0], "EXIT") == 0){
        close(client_fd);
        return 0;
    }
    else {
        return 1;
    }
    snprintf(send, sizeof(send), "%d\n", *AC);
    write(client_fd, send, strlen(send));

    return 1;
}
