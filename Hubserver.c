#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#define MAX_CONNECTION_NUM 64

SOCKADDR_IN known_connections[MAX_CONNECTION_NUM];
int connections = 0;

void handleSetup(WSADATA *wsadata, SOCKET *main_socket, SOCKADDR_IN *addr){
    int error_code = WSAStartup(MAKEWORD(2, 2), wsadata);
    if( error_code != 0){
        printf("WSAStartup failed with the error %s\n", error_code);
        exit(EXIT_FAILURE);
    }
    printf("WSAStartup Successful!\n");

    *main_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if(*main_socket == INVALID_SOCKET){
        printf("ERROR CREATING SOCKET\n\tERROR CODE: %d\n", WSAGetLastError());
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    printf("Socket created successfully!\n");

    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr("192.168.1.13");
    addr->sin_port = htons(25565);

    if( bind(*main_socket, (SOCKADDR *) addr, sizeof(*addr)) == SOCKET_ERROR){
        printf("Error binding socket: CODE %d\n", WSAGetLastError());
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    printf("Socket binded successfully!\n");
    return;
}

int handleIntroductoryCheck(char *buf, SOCKADDR_IN *incoming_socket){
    for(int i=0; i<connections; i++){
        if(known_connections[i].sin_addr.s_addr == incoming_socket->sin_addr.s_addr)
            return 0;
    }
    return 1;
}

void handleReceivedData(char *buf, SOCKET *main_socket, SOCKADDR_IN *incoming_socket, int *recv_len, int *slen){
    if(strcmp(buf, "talk to me shawty") == 0 && handleIntroductoryCheck(buf, incoming_socket) == 1){
            printf("We got an introductory request\nAdding IP to known_connections");
            known_connections[connections] = *incoming_socket;
            connections++;
    } else {
        int index = 0;
        while( ntohs(known_connections[index].sin_port) != 0 && index < connections && index < MAX_CONNECTION_NUM){
            SOCKADDR_IN current_connection = known_connections[index];

            if(ntohl(current_connection.sin_addr.s_addr) != ntohl(incoming_socket->sin_addr.s_addr)){
                if( sendto(*main_socket, buf, *recv_len, 0, (struct sockaddr*) &current_connection, *slen) == SOCKET_ERROR ){
                    printf("ERROR SENDING DATA TO CLIENT %s\nERROR CODE:%d", inet_ntoa(current_connection.sin_addr), WSAGetLastError());
                    index++;
                } else {
                    printf("Data sent to %s was successful!\n", inet_ntoa(current_connection.sin_addr));
                }
            } else {
                printf("skipping IP because it is the original sender of the datagram\n");
            }
            index++;
        }
    }
}

int main(int argc, char *argv[]){
    WSADATA wsadata;
    SOCKET main_socket;
    SOCKADDR_IN addr, incoming_socket;
    handleSetup(&wsadata, &main_socket, &addr);

    int recv_len, slen = sizeof(incoming_socket);
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
        handleReceivedData(buf, &main_socket, &incoming_socket, &recv_len, &slen);
        free(buf);
        free(&incoming_socket);
    }
}