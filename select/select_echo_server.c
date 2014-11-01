#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#define LISTENQ 5

int main(int argc, char* argv[]){
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
    listen(listenfd, LISTENQ);

    int maxfd = listenfd; /* initialize */
    int maxi = -1; /* index into client[] array */
    fd_set rset, allset;
    int nready, client[FD_SETSIZE];
    int i;
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

        rset = allset; /* structure assignment */
        nready = select(maxfd+1, &rset, NULL, NULL, NULL);

        if(FD_ISSET(listenfd, &rset)){
            socklen_t clilen = sizeof(cliaddr);

            /* accept a client from the listening fd */
            client_fd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
            printf("A client connect in\n");
            for(i=0; i<FD_SETSIZE; i++){
                if(client[i]<0){
                    client[i] = client_fd; /* save descriptor */
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

                else if(recv_len == 0){
                    printf("The client close the connection\n");
                    close(sock_fd);
                    FD_CLR(sock_fd, &allset);
                    client[i] = -1;
                }
                else {
                    printf("Receive message from the client, message is %s\n", recv_buf);
                    write(sock_fd, recv_buf, strlen(recv_buf));
                }

                if(--nready <= 0){ /* no more readable descriptors */
                    break;
                }
            }
        }
    }
}
