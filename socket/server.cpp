#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <cstring>
#include <cstdlib>

const int RETURN_ERRNO = 1;
const int DEF_PORT_NO = 20000;
const int CONNECT_LIMIT = 10;
const int BUFFER_LEN = 1024;

using namespace std;
int main(int argc, char* argv[]){
    sockaddr_in self_addr, client_addr;
    socklen_t client_addr_len;
    int recv_len;
    int server_sock, temp_sock;
    char buffer[BUFFER_LEN];

    //Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock == -1){
        cout << "Cannot create socket." <<endl;
        return RETURN_ERRNO;
    }

    //Read port
    string new_port;
    cout << "Enter Server Port Number(Empty for default:" << DEF_PORT_NO << "):";
    getline(cin, new_port);
    int final_port;
    if(new_port.size() <= 1){
        final_port = DEF_PORT_NO;
    }
    else {
        final_port = atoi(new_port.c_str());
    }

    //Bind to port
    if(bind(server_sock, (sockaddr*)&self_addr, sizeof(self_addr)) == -1){
        cout << "Cannot bind to port." << endl;
        return RETURN_ERRNO;
    }

    //Listen the port
    if(listen(server_sock, CONNECT_LIMIT) == -1){
        cout << "Cannot listen the port." << endl;
        return RETURN_ERRNO;
    }
    cout << "Listen... OK" << endl;

    while(true){
        cout << "Waiting for connection..." <<endl;

        //Accept
        temp_sock = accept(server_sock, (sockaddr*)&client_addr, &client_addr_len);
        if(temp_sock == -1){
            cout << "Cannot accept request from client." << endl;
            return RETURN_ERRNO;
        }
    }

    cout << "A client has connected with sockfd:" << temp_sock << endl;
    
    //Recieve data
    bzero(buffer, BUFFER_LEN);
    int recv_data_len = recv(temp_sock, buffer, BUFFER_LEN-1, 0);
    if(recv_data_len <0){
        cout << "Recieve Data Fail." << endl;
        return RETURN_ERRNO;
    }
    cout << "Recieved" << recv_data_len << "bytes from client." << endl;
    cout << "Contents:" << buffer << endl;

    //Send data back
    string reply_str = string("You say:")+string(buffer);
    send(temp_sock, reply_str.c_str(), reply_str.length()+1, 0);

    //Close connection
    close(temp_sock);
    cout << "Close connection." << endl;

}
