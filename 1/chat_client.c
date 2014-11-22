#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>

int main(int argc, char* argv[]){
    struct sockaddr_in servaddr;
    struct hostent *host;
    int fd;
    int recv_len;
    char recv_buf[1024];
    char send_buf[1024];

    if(argc != 3){
        printf("Usage: client <IP addr> <port> \n");
        return 0;
    }

    /* Set data of servaddr to 0 */
    memset(&servaddr, 0, sizeof(servaddr)); 

    /* get the host information by the name(ip address) */
    if((host = gethostbyname(argv[1])) == NULL){
        printf("ERROR: Get host by name error\n");
        return 0;
    }

    servaddr.sin_family = PF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    
    memcpy(&servaddr.sin_addr, host->h_addr, host->h_length);

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("ERROR: Create socket error\n");
        return 0;
    }

    /* Connect to the server in the servaddr */
    if(connect(fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        printf("ERROR: Connect to the server error\n");
        return 0;
    }
    
    int maxfdp1, stdineof = 0;
    fd_set rset;
    FD_ZERO(&rset);

    /* main loop */
    while(1){
        if(stdineof == 0)
            FD_SET(fileno(stdin), &rset);
        FD_SET(fd, &rset);
        maxfdp1 = ((fileno(stdin) > fd)? fileno(stdin) : fd) + 1;
        select(maxfdp1, &rset, NULL, NULL, NULL);

        if(FD_ISSET(fd, &rset)){ /* socket is readable */
            /* Read from the server */
            memset(&recv_buf, 0, sizeof(recv_buf)); 
            recv_len = read(fd, recv_buf, sizeof(recv_buf));

            if(recv_len == 0){
                printf("Server close the connection\n");
                return 0;
            }
            else if(recv_len < 0){
                printf("ERROR: Read from the server error\n");
                return 0;
            }
            else {
                printf("%s", recv_buf);
            }
        }

        if(FD_ISSET(fileno(stdin), &rset)){ /* input is readable */
            /* Read message from the stdin, and send it to the server */
            memset(&send_buf, 0, sizeof(send_buf)); 
            if(fgets(send_buf, sizeof(send_buf), stdin) == NULL){
                stdineof = 1;
                shutdown(fd, SHUT_WR); /* send FIN */
                FD_CLR(fileno(stdin), &rset);
                continue;
            }
            if(strcmp(send_buf, "exit\n") == 0){
                close(fd);
                return 0;
            }
            write(fd, send_buf, strlen(send_buf));
        }
    }
}
