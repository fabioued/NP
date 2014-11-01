#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <string>
#include <cstring>

const int RETURN_ERRNO = 1;
const int DEF_PORT_NO = 20000;
const char* DEF_IP_ADDR = "140.113.235.132";
const int BUFFER_LEN = 1024;
using namespace std;

int main(int argc, char* argv[]){
    //Read Server Address
    cout << "Enter Server IP Address(Empty for default: " << DEF_IP_ADDR << "):";
    string new_ip;
    const char* final_ip;
    getline(cin, new_ip);
    if(new_ip.size() <= 1){
        final_ip = DEF_IP_ADDR;
    }
    else {
        final_ip = new_ip.c_str();
    }

    //Read port
    string new_port;
    cout << "Enter Server Port Number (Empty for default:" << DEF_PORT_NO << "):";
    getline(cin, new_port);
    int final_port;
    if(new_port.size() <= 1){
        final_port = DEF_PORT_NO;
    }
    else{
        final_port = atoi(new_port.c_str());
    }

    string str;
    char buffer[BUFFER_LEN];
    while(cout << "Enter something to echo(empty line to terminate):", getline(cin, str), str.size()>0){
        //Create socket
        int client_sock = socket(AF_INET, SOCK_STREAM, 0);
        if(client_sock == -1){
            cout << "Cannot create socket." << endl;
            return RETURN_ERRNO;
        }

        //Initiallize sockaddr object
        sockaddr_in server_addr;
        bzero(&server_addr, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(final_ip);
        server_addr.sin_port = htons(final_port);
        
        //Ready to connect
        cout << "Connect to " << final_ip << ":" << final_port << "...";
        if(connect(client_sock,(sockaddr*)&server_addr,sizeof(server_addr)) < 0){
            cout << "Failed..." << endl;
            return RETURN_ERRNO;
        }
        else {
            cout << "Success!" << endl;
        }
        int send_result = send(client_sock, str.c_str(), str.size(), 0);
        if(send_result < 0){
            cout << "Cannot send request" << endl;
            return RETURN_ERRNO;
        }
        bzero(buffer, BUFFER_LEN);
        int recv_result = recv(client_sock, buffer, BUFFER_LEN-1, 0);
        if(recv_result < 0){
            cout << "Cannot recieve data from server." << endl;
            return RETURN_ERRNO;
        }
        cout << "Server reply:" << buffer << endl;
        cout << "Terminate Connection." << endl;
        close(client_sock);
    }

    return 0;
}
