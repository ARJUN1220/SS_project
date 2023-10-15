#include <stdio.h> 
#include <errno.h> 
#include <fcntl.h>     
#include <unistd.h>    
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netinet/ip.h> 
#include <string.h>  
#include <stdbool.h> 
#include <stdlib.h> 

#include "./functions/server-constants.h"
#include "./functions/admin.h"
#include "./functions/student.h"
#include "./functions/faculty.h"

void connection_handler(int connectionFileDescriptor)
{
    printf("Client has connected to the server!\n");

    char readBuffer[1000], writeBuffer[1000];
    ssize_t readBytes, writeBytes;
    int userChoice;
    writeBytes = write(connectionFileDescriptor, INITIAL_PROMPT, strlen(INITIAL_PROMPT));
    if (writeBytes == -1)
        perror("Error while sending first prompt to the user!");
    else
    {
        bzero(readBuffer, sizeof(readBuffer));
        readBytes = read(connectionFileDescriptor, readBuffer, sizeof(readBuffer));
        if (readBytes == -1)
            perror("Error while reading from client");
        else if (readBytes == 0)
            printf("No data was sent by the client");
        else
        {
            userChoice = atoi(readBuffer);
            switch (userChoice)
            {
            case 1:
                // Admin
                admin_operation_handler(connectionFileDescriptor);
                break;
            case 2:
                // Faculty
                faculty_operation_handler(connectionFileDescriptor);
                break;
            case 3:
                // Student
                student_operation_handler(connectionFileDescriptor);
                break;
            default:
                // Exit
                break;
            }
        }
    }
    printf("Terminating connection to client!\n");
}

void main()
{
    int socketFileDescriptor, socketBindStatus, socketListenStatus, connectionFileDescriptor;
    struct sockaddr_in serverAddress, clientAddress;

    socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if(socketFileDescriptor == -1) {
        perror("Error while creating server socket!");
        _exit(0);
    }

    serverAddress.sin_family = AF_INET;                
    serverAddress.sin_port = htons(8080);             
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    socketBindStatus = bind(socketFileDescriptor, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if(socketBindStatus == -1) {
        perror("Error while binding to server socket!");
        _exit(0);
    }

    socketListenStatus = listen(socketFileDescriptor, 10);
    if(socketListenStatus == -1) {
        perror("Error while listening for connections on the server socket!");
        close(socketFileDescriptor);
        _exit(0);
    }

    int clientSize;
    while(1) {
        clientSize = (int)sizeof(clientAddress);
        connectionFileDescriptor = accept(socketFileDescriptor, (struct sockaddr *)&clientAddress, &clientSize);
        if(connectionFileDescriptor == -1) {
            perror("Error while connecting to client!");
            close(socketFileDescriptor);
        }
        else {
            if(!fork()) {
                connection_handler(connectionFileDescriptor);
                close(connectionFileDescriptor);
                _exit(0);
            }
        }
    }
    close(socketFileDescriptor);
}
