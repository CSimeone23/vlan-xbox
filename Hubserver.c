#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

SOCKADDR_IN known_connections[64];
int connection = 0;

int main(int argc, char *argv[]){
    WSADATA wsadata;
    int err_code = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if(err_code != 0){
        printf("WSAStartup failed with the error %s\n", err_code);
        exit(EXIT_FAILURE);
    }

    SOCKET main_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(main_socket == INVALID_SOCKET){
        printf("ERROR CREATING SOCKET\n\tERROR CODE: %d\n", WSAGetLastError());
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("192.168.1.13");
    addr.sin_port = htons(25565);
    int binder = bind(main_socket, (SOCKADDR *) &addr, sizeof(addr));
    if(binder == SOCKET_ERROR){
        printf("ERROR BINDING SOCKET: CODE %d", WSAGetLastError());
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    SOCKADDR_IN incoming_socket;
    int recv_len;
    int slen = sizeof(incoming_socket);
    while(TRUE){
        printf("Waiting for data...\n");
        char *buf = calloc(512, sizeof(char));
        recv_len = recvfrom(main_socket, buf, 512, 0, (struct sockaddr*) &incoming_socket, &slen);
        if(recv_len == SOCKET_ERROR){
            printf("ERROR RECEIVING DATA FROM CLIENT:\n\tERROR CODE: %d\n", WSAGetLastError());
            continue;
        }
        printf("Received packet from %s:%d\n", inet_ntoa(incoming_socket.sin_addr), ntohs(incoming_socket.sin_port));
        printf("Data: %s\n", buf);

        if(strcmp(buf, "talk to me shawty") == 0){
            printf("We got an introductory request\nAdding IP to known_connections");
            known_connections[connection] = incoming_socket;
            connection++;
        } else {
            int index = 0;
            while( ntohs(known_connections[index].sin_port) != 0 && index < 64){
                SOCKADDR_IN current_connection = known_connections[index];

                if(ntohl(current_connection.sin_addr.s_addr) != ntohl(incoming_socket.sin_addr.s_addr)){
                    int send_to = sendto(main_socket, buf, recv_len, 0, (struct sockaddr*) &current_connection, slen);
                    if( send_to == SOCKET_ERROR ){
                        printf("ERROR SENDING DATA TO CLIENT %s\nERROR CODE:%d", inet_ntoa(current_connection.sin_addr), WSAGetLastError());
                        index++;
                        continue;
                    }
                    printf("Data sent to %s was successful!\n", inet_ntoa(current_connection.sin_addr));
                } else {
                    printf("skipping IP because it is the original sender of the datagram\n");
                }
                index++;
            }
        }
        free(buf);
        free(&incoming_socket);
    }
}