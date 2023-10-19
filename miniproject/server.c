/*
This file consists of code which listens for client connections and connect to it through socket. Used forking for handling multiple clients  
*/


#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<string.h>

#include "./functions/server-constants.h"
#include "./functions/admin.h"
#include "./functions/faculty.h"
#include "./functions/student.h"


// function which handles client requests
int handle_client(int client_socket,int i) {
    int userchoice;
    char received[1000];
    char response[1024];
    int bytes_received;
    
    //displaying intial prompt
    write(client_socket,INITIAL_PROMPT,strlen(INITIAL_PROMPT));
    bzero(received,sizeof(received));
    bytes_received = read(client_socket,received,sizeof(received));
    if(bytes_received<0)
      printf("Error while reading from client");
    else if(bytes_received==0)
      printf("No data sent from client");
    else{
       userchoice = atoi(received);
       switch(userchoice){
        case 1:
              // Admin
               admin_operation_handler(client_socket);
               break;
        case 2:
              // Professor
                faculty_operation_handler(client_socket);
                break;
        case 3:
              // Student
                student_operation_handler(client_socket);
                break;         
        default:
                write(client_socket,"wrong choice ^",14);
                char readBuffer[100];
                int readBytes = read(client_socket,readBuffer,sizeof(readBuffer));
                break;
       }
    }    
    close(client_socket);
    return 1;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_addr_len = sizeof(client_address);

    // Create a socket
    server_socket = socket(AF_INET,SOCK_STREAM,0);
    if(server_socket<0){
       perror("socket returns");
       return 1;
    }

   // memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8080); 

    // Bind the socket to the server address
    if(bind(server_socket,(struct sockaddr*)&server_address,sizeof(server_address))<0){
       perror("bind returns");
       close(server_socket);
       return 1;
    }

    // Listen for incoming connections
    if(listen(server_socket,5)<0){
       perror("listen returns");
       close(server_socket);
       return 1;
    }

    printf("Server listening for connections....\n");
    int number=1;
    
    while(1){
      // Accept a client connection
      client_socket = accept(server_socket,(struct sockaddr*)&client_address,&client_addr_len);
      if(client_socket<0){
         perror("accept returns");
         continue; // Continue to accept other connections
      }

      // Fork a child process to handle the client
      pid_t child_pid = fork();
      if(child_pid == 0){
         close(server_socket); // Close the server socket in the child process
         handle_client(client_socket,number); // Handle the client
         number=number+1;
         return 1; 
      } 
      else if(child_pid>0){
        close(client_socket); // Close the client socket in the parent process
      } 
      else{
        perror("fork returns");
        close(client_socket); // Close the client socket in case of fork failure
      }
    }

    // Close the server socket
    close(server_socket);

    return 0;
}
