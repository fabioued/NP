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
#include <sys/wait.h>
#include <sys/signal.h>

int client_num, flag;
int cal(int client_fd, char* buf, int *AC, char *send);
void sig_chld(int);

int main(int argc, char* argv[]){
    client_num = 0;
    signal(SIGCHLD, sig_chld);

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
        socklen_t clilen = sizeof(cliaddr);
        
        if(client_num == 0)
            flag = 0;
        else if(client_num >= 5)
            flag = 1;

        if(flag == 0){
            int AC = 0;
            /* accept a client from the listening fd */
            if((client_fd = accept(listenfd, NULL, NULL)) < 0){
                printf("ERROR: Accept client error\n");
                close(listenfd);
                return 0;
            }
            pid_t pid = fork();
            
            if(pid == 0){
                close(listenfd);
                while(1){
                    /* read some message from the client */
                    memset(recv_buf, 0, sizeof(recv_buf));
                    memset(send_buf, 0, sizeof(send_buf));

                    recv_len = read(client_fd, recv_buf, sizeof(recv_buf));
                    if(recv_len < 0){
                        printf("ERROR: Read from client error\n");
                        close(client_fd);
                        exit(1);
                    }

                    else if(recv_len == 0){
                        printf("The client close the connection\n");
                        close(client_fd);
                        exit(1);
                    }
                    else {
                        printf("Receive message from the client, message is %s\n", recv_buf);
                        int result = cal(client_fd, recv_buf, &AC, send_buf);
                        if(result == 0)
                            exit(1);
                    }
                }
            }
            else if(pid > 0){
                close(client_fd);
                client_num++;
                if(client_num == 0)
                    flag = 0;
                else if(client_num >= 5)
                    flag = 1;
                printf("A client connect in, client: %d\n", client_num);
            }
        }
        else{
            int AC[FD_SETSIZE];
            rset = allset;
            nready = select(maxfd+1, &rset, NULL, NULL, NULL);

            if(FD_ISSET(listenfd, &rset)){
                /* accept a client from the listening fd */
                client_fd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
                printf("A client connect in, client: %d\n", client_num);
                client_num++;
                if(client_num == 0)
                    flag = 0;
                else if(client_num >= 5)
                    flag = 1;

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
                        client_num--;
                        if(client_num == 0)
                            flag = 0;
                        else if(client_num >= 5)
                            flag = 1;
                        break;
                    }

                    else if(recv_len == 0){
                        printf("The client close the connection\n");
                        close(client[i]);
                        FD_CLR(client[i], &allset);
                        client[i] = -1;
                        client_num--;
                        if(client_num == 0)
                            flag = 0;
                        else if(client_num >= 5)
                            flag = 1;
                        break;
                    }
                    else {
                        printf("Receive message from the client, message is %s\n", recv_buf);
                        int result = cal(client[i], recv_buf, (AC+i), send_buf);
                        if(result == 0){
                            FD_CLR(client[i], &allset);
                            client[i] = -1;
                            client_num--;
                            if(client_num == 0)
                                flag = 0;
                            else if(client_num >= 5)
                                flag = 1;
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

void sig_chld(int signo){
    int stat;
    while(waitpid(-1, &stat, WNOHANG) > 0){
        printf("Client has closed the connection.\n");
        client_num--;
        if(client_num == 0)
            flag = 0;
        else if(client_num >= 5)
            flag = 1;
    }
}
