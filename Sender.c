#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define PORT 8080
#define IP_ADDRESS "127.0.0.1"
#define FILE_SIZE 2097148
#define FILE_NAME "TextFile.txt"
#define ID1 2781
#define ID2 8413

size_t send_message(char[], int);

int main() {
    //-------------------------------Read File-----------------------------
    FILE* file_pointer;
    file_pointer = fopen(FILE_NAME, "r");

    // Check if we were successful in opening file.
    if(file_pointer == NULL) {
        printf("(-) Error in opening file! -> fopen() failed with error code: %d\n", errno);
        exit(EXIT_FAILURE); // Exit program and return EXIT_FAILURE (defined as 1 in stdlib.h).
    }
    else {
        printf("(=) File opened successfully.\n");
    }

    // Create array for holding the file.
    char message[FILE_SIZE + 1] = {0}; // file size + 1 for the \0.
    // Read from file to "message".
    fread(message, sizeof(char), FILE_SIZE, file_pointer);

    // Closes the stream. All buffers are flushed.
    fclose(file_pointer);

    //-------------------------------Create TCP Connection-----------------------------
    // Creates socket named "socketFD". FD for file descriptor.
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);

    // Check if we were successful in creating socket.
    if(socketFD == -1) {
        printf("(-) Could not create socket! -> socket() failed with error code: %d\n", errno);
        exit(EXIT_FAILURE); // Exit program and return EXIT_FAILURE (defined as 1 in stdlib.h).
    }
    else {
        printf("(=) Socket created successfully.\n");
    }

    // Create sockaddr_in for IPv4 for holding ip address and port and clean it.
    struct sockaddr_in serverAddress;
    memset(&serverAddress, '\0', sizeof(serverAddress));

    // Assign port and address to "serverAddress".
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT); // Short, network byte order.
    serverAddress.sin_addr.s_addr = inet_addr(IP_ADDRESS);

    // Convert address to binary.
    if (inet_pton(AF_INET, IP_ADDRESS, &serverAddress.sin_addr) <= 0)
    {
        printf("(-) Failed to convert IPv4 address to binary! -> inet_pton() failed with error code: %d\n", errno);
        exit(EXIT_FAILURE); // Exit program and return EXIT_FAILURE (defined as 1 in stdlib.h).
    }

    //Create connection with server.
    int connection = connect(socketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress));

    // Check if we were successful in connecting with server.
    if(connection == -1) {
        printf("(-) Could not connect to server! -> connect() failed with error code: %d\n", errno);
        exit(EXIT_FAILURE); // Exit program and return// EXIT_FAILURE (defined as 1 in stdlib.h).
    }
    else {
        printf("(=) Connection with server established.\n\n");
    }

    //-------------------------------Send Message---------------------------------
    int ans;
    while (true) {

        //----------------------------------Change CC To Cubic-----------------------------------------------
        if (setsockopt(socketFD, IPPROTO_TCP, TCP_CONGESTION, "cubic", 5) == -1) {
            printf("(-) Failed to change cc algorithm! -> setsocketopt() failed with error code: %d\n", errno);
        }
        else {
            printf("(+) Set CC algorithm to: cubic\n");
        }

        //----------------------------Send First Half------------------------------
        if (send_message(message, socketFD) == -1) {
            printf("(-) Failed to send first half of message! -> send() failed with error code: %d\n", errno);
        }
        else {
            printf("(+) Sent the first half of the message.\n");
        }

        //-------------------------------Receive Authentication---------------------------------
        char msg[6] = {0}; // Array for holding the xor result from receiver. It's of size 5. +1 for the \0.
        if (recv(socketFD, msg, 5, 0) <= 0) { // Check if we got an error (-1) or peer closed half side of the socket (0).
            printf("(-) Error in receiving data or peer closed half side of the socket.");
        }
        char xor[6] = {0}; // Array of holding the xor result. It's of size 5. +1 for the \0.
        sprintf(xor, "%d", ID1 ^ ID2); // Convert int (xor result) to string and put in "xor" array.
        if (strncmp(xor, msg, 5) == 0) {
            printf("(+) Authentication was successful...\n");
        } else {
            printf("(-) Authentication failed...\n");
        }

        //----------------------------------Change CC To Reno-----------------------------------------------
        if (setsockopt(socketFD, IPPROTO_TCP, TCP_CONGESTION, "reno", 4) == -1) {
            printf("(-) Failed to change cc algorithm! -> setsocketopt() failed with error code: %d\n", errno);
        }
        else {
            printf("(+) Set CC algorithm to: reno\n");
        }

        //----------------------------Send Second Half------------------------------
        if(send_message(message + FILE_SIZE / 2, socketFD) == -1) {
            printf("(-) Failed to send second half of message! -> send() failed with error code: %d\n", errno);
        }
        else {
            printf("(+) Sent the second half of the message.\n\n");
        }

        printf("Do you want to send file? Enter Y for Yes or N for No.\n"); // User decision.
        while((ans = getchar()) == '\n' || getchar() == EOF);
        if (ans == 'N') {
            send(socketFD, "exit", 4, 0); // Send exit message to the receiver.
            break;
        }
        printf("-----------------------------------------------------------------\n");
    }
    printf("(=) Exiting...\n");

    //-------------------------------Close Connection-----------------------------
    if(close(socketFD) == -1) {
        printf("(-) Failed to close connection! -> close() failed with error code: %d\n", errno);
    }
    else {
        printf("(=) Connection closed!\n");
    }

    return 0;
}

// Method for sending halves of message.
size_t send_message(char message[], int socketFD) {
    size_t totalLengthSent = 0; // Variable for keeping track of number of bytes sent.
    while (totalLengthSent < FILE_SIZE/2) {
        ssize_t bytes = send(socketFD, message + totalLengthSent, FILE_SIZE/2 - totalLengthSent, 0);
        if (bytes == -1) {
            return -1;
        }
        totalLengthSent += bytes;
    }
    return 1;
}