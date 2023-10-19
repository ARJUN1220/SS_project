/*
This file consists of client code which connects to the server and give inputs for further processing   
*/


#include <sys/socket.h> 
#include <netinet/ip.h> 
#include <stdio.h>      
#include <unistd.h>
#include <string.h>  

int connection_handler(int connFD);
void main(){
    int socketFileDescriptor; 
    int connectStatus; 

    struct sockaddr_in address;

    ssize_t readBytes, writeBytes;
    char dataFromServer[1024];
    int choice;

    // Create an endpoint for communicate -> here, create the client side point
    // Create a socket for TCP connection using IPv4
    socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFileDescriptor == -1)
    {
        perror("Error while creating socket!");
        _exit(0);
    }

    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);

    connectStatus = connect(socketFileDescriptor, (struct sockaddr *)&address, sizeof(address));
    if (connectStatus == -1)
    {
        perror("Error while connecting to server!");
        _exit(0);
    }
    printf("Client to server connection successfully established!\n");
   
    connection_handler(socketFileDescriptor);
    close(socketFileDescriptor);
}

int connection_handler(int sockFD)
{
    char readBuffer[1000], writeBuffer[1000]; 
    ssize_t readBytes, writeBytes;           
    char tempBuffer[1000];

    do
    {
        bzero(readBuffer, sizeof(readBuffer)); // Empty the read buffer
        bzero(tempBuffer, sizeof(tempBuffer));
        
        readBytes = read(sockFD, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
            perror("Error while reading from client socket!");
        else if (readBytes == 0){
            return 0;
        }
        else if (strchr(readBuffer, '^') != NULL)
        {
            // Skip read from client
            strncpy(tempBuffer, readBuffer, strlen(readBuffer) - 1);
            printf("%s\n", tempBuffer);
            writeBytes = write(sockFD, "^", strlen("^"));
            if (writeBytes == -1)
            {
                perror("Error while writing to client socket!");
                break;
            }
        }
        else if (strchr(readBuffer, '$') != NULL)
        {
            // Server sent an error message and is now closing it's end of the connection
            strncpy(tempBuffer, readBuffer, strlen(readBuffer) - 2);
            printf("%s\n", tempBuffer);
            printf("Closing the connection to the server now!\n");
            break;
        }
        else
        {
            bzero(writeBuffer, sizeof(writeBuffer)); // Empty the write buffer

            if (strchr(readBuffer, '#') != NULL)
                strcpy(writeBuffer, getpass(readBuffer));
            else{
               printf("%s", readBuffer);
               scanf("%[^\n]%*c", writeBuffer); // Take user input!
            }
               writeBytes = write(sockFD, writeBuffer, strlen(writeBuffer));
               if (writeBytes == -1){
                 perror("Error while writing to client socket!");
                 printf("Closing the connection to the server now!\n");
                 break;
               }
            }
    } while(readBytes > 0);

    close(sockFD);
}
