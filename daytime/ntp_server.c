#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

int main(int argc, char* argv[]){
    struct sockaddr_in servaddr;
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

    while(1){
        /* main loop */
        int client_fd;
        char buf[64];

        /* accept a client from the listening fd */
        if((client_fd = accept(listenfd, NULL, NULL)) < 0){
            printf("ERROR: Accept client error\n");
            close(listenfd);
            return 0;
        }

        /* read some message from the client */
        /* the read syscall returns 0 if the client close the connection */
        /* returns -1 if some error occurs */
        /* or returns the length of message we read */
        memset(buf, 0, sizeof(buf));
        time_t t1 = time(NULL);
        sprintf(buf, "%s", ctime(&t1));
        write(client_fd, buf, strlen(buf));
        close(client_fd); 
    }
}
