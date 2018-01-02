/*
    C ECHO client example using sockets
*/
#include<stdio.h> //printf
#include<string.h>    //strlen
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr

#define PKT_SIZE 60

int main(int argc , char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char message[PKT_SIZE] , server_reply[PKT_SIZE];

    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1){
        printf("Could not create socket");
    }
    puts("Socket created");

    server.sin_addr.s_addr = inet_addr("10.218.111.254");
    server.sin_family = AF_INET;
    server.sin_port = htons( 80 );

    //Connect to remote server
    // int ff_connect(int s, const struct linux_sockaddr *name, socklen_t namelen);
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }

    puts("Connected\n");

    //keep communicating with server
    while(1)
    {
        printf("Enter message : ");
        scanf("%s" , message);

        //Send some data
        memset( message, '\0', sizeof(char));
        if( send(sock , message , strlen(message) , 0) < 0)
        {
            puts("Send failed");
            return 1;
        }

        //Receive a reply from the server
        memset( server_reply, '\0', sizeof(char));
        if( recv(sock , server_reply , PKT_SIZE , 0) < 0){
            puts("recv failed");
            break;
        }

        puts("Server reply :");
        puts(server_reply);
    }

    close(sock);
    return 0;
}