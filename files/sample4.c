/**
 * Skeleton file for server.c
 *
 * You are free to modify this file to implement the server specifications
 * as detailed in Assignment 3 handout.
 *
 * As a matter of good programming habit, you should break up your imple-
 * mentation into functions. All these functions should contained in this
 * file as you are only allowed to submit this file.
 */

#include <stdio.h>
// Include necessary header files
#include<sys/socket.h> //socket
#include<netinet/in.h> // socket address struct sockaddr_in
#include<string.h> //copy string
#include<ctype.h>  // for toupper
#include<stdlib.h> //exit()
#include<unistd.h> //read and write

/**
 * Write from the server to the client
 * incoming = message
 * client = who to send it to
 */
void writeTo(char *incoming, int client){
  char buffer[10000] = {0};
  strncpy(buffer, incoming, strlen(incoming));
  int s = write(client, buffer, 10000); // writing to the client
  if(s<0){
      printf("Write error");
  }
  printf("Write successful\n");
}


/**
 * Open the file (filename) to read
 * Send the file contents back to the client
 */
void getFile(char *filename, int clientId){
  FILE *file;
  file = fopen(filename, "r");

  // If the file could not open:
  if(file == NULL){
    writeTo("SERVER 404 Not Found\n", clientId);
    return;
  }

  char fileContents[10000] = {0};
  char c;
  int counter = strlen("SERVER 200 OK\n\n");
  strncpy(fileContents, "SERVER 200 OK\n\n", counter);

  while((c = fgetc(file)) != EOF){
    strcpy(&fileContents[counter], &c);
    counter++;
  }

  // Add 3 new line characters onto the end
  for(int i = 0; i < 3; i++){
    strcpy(&fileContents[counter], "\n");
    counter++;
  }

  // Send the content back to the client:
  writeTo(fileContents, clientId);
  fclose(file);
}

/**
 * Open the file (filename) to write
 * clear the original contents of the file
 * write the incoming contents to the file
 */
void putFile(char *filename, int clientId){
  FILE *file;
  file = fopen(filename, "w");

  // If the file could not open:
  if(file == NULL){
    writeTo("SERVER 501 Put Error\n", clientId);
    return;
  }

  // Empty contents of original file:
  char c;
  while((c = fgetc(file)) != EOF){
    fputc (NULL, file);
  }

  int prevNull = 0;
  int closeFile = 0;
  while(1){
    // Get the new input for the file:
    char userInput[10000] = {0};
    int re = recv(clientId, &userInput, 10000, 0);
    if(re < 0) {
      printf("Error receiving message\n");
    }
    printf("Received file input: %s \n", userInput);

    int userInputSize = sizeof(userInput)/sizeof(char);
    for(int i = 0; i < userInputSize; i++){
      fputc(userInput[i], file);
      if(i > 0){
        if(prevNull){
          if(!(userInput[i] == 'n' || userInput[i] == '\\')){
            prevNull = 0;
          }
        }
        if(userInput[i] == 'n' && userInput[i-1] == '\\'){
          if(prevNull){
            printf("Found 2 new line char \n");
            closeFile = 1;
            fclose(file);
            break;
          }
          prevNull = 1;
        }
      }
    }
    if(closeFile){
      break;
    }
  }
  writeTo("SERVER 201 Created\n", clientId);
}


/**
 * Make sure that there is not a '\n' at the end of the file name
 */
void editFileName(char *incoming, int isGet, int clientId){
  char filename[1000] = {0};

  // If they did not enter a file name:
  if(strlen(incoming) <= 4){
    if(isGet){
      writeTo("SERVER 500 Get Error\n", clientId);
      return;
    }else{
      writeTo("SERVER 501 Put Error\n", clientId);
      return;
    }
  }

  // Get the file name from the incoming
  for(int i = 4; i < 1000; i++){
    if(incoming[i] == '\n'){
      break;
    }
    filename[i-4] = incoming[i];
  }

  if(isGet){
    getFile(&filename, clientId);
  }else{
    putFile(&filename, clientId);
  }
}


/**
 * The main function should be able to accept a command-line argument
 * argv[0]: program name
 * argv[1]: port number
 *
 * Read the assignment handout for more details about the server program
 * design specifications.
 */
int main(int argc, char *argv[])
{
    int sockfd;

    if(argc == 1 || atoi(argv[1]) < 1024){
      printf("Error initialising port number.");
      return -1;
    }
    int port = atoi(argv[1]);

    // Create the Socket
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0){
        printf("Error creating socket");
    }
    printf("Socket Created\n");

    //define IP and Port
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr= INADDR_ANY; // any Address
    serveraddr.sin_port=htons(port); // Port number, htons for endian

    printf("Address created\n");

    //bind address to socket
     int br;
     br = bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
     if(br<0){
         printf("Error Binding");
    }
    printf("Bind successful\n");

    //listen for connection requests
    if(listen(sockfd,5)<0){
          printf("Litening error");
    }
    printf("Success listening\n");

    int running = 1;
    while(running){
      struct sockaddr_in clientaddr;
      int clienlen = sizeof(clientaddr);

      int clientfd = accept(sockfd,(struct sockaddr*)&clientaddr,(socklen_t*)&clientaddr);

      if(clientfd<0){
          printf("Error accepting client");
      }
      printf("Client connection accepted\n");

      int forkId = fork(); // Make a child process
      if(forkId == -1){
        printf("Forking Error");
        // Will just continue with the parent process
      }

      if(forkId == 0){
        printf("You are the child process \n");
      }else{
        // Make the parent wait until the child process has finished executing
        wait(NULL);
        printf("You are the parent process \n");
      }

      writeTo("HELLO\n", clientfd);

      while(1){
        //read from client
        char incoming[100] = {0};
        char incomingUpper[100] = {0}; // same as incoming but all upper case

        int r = recv(clientfd, &incoming, 100, 0);
        if(r < 0) {
          printf("Error receiving message\n");
        }
        printf("Received message: %s \n", incoming);

        int incoming_len = sizeof(incoming)/sizeof(char);

        // Convert all to upper, so that it is case insensitive
        for(int i = 0; i < 100; i++){
          incomingUpper[i] = toupper(incoming[i]);
        }


        if(strncmp(incomingUpper, "BYE", 3) == 0){
          printf("Closing connection. \n");
          if(forkId == 0){
            exit(0); // terminate the child process
          }else{
            close(clientfd);
            running = 0;
          }
          break;


        }else if(strncmp(incomingUpper, "GET", 3) == 0){
          editFileName(&incoming, 1, clientfd);

        }else if(strncmp(incomingUpper, "PUT", 3) == 0){
          editFileName(&incoming, 0, clientfd);

        }
      }
    }
    close(sockfd);

    return 0;
}
