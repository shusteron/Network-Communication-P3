#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <signal.h>
#include <arpa/inet.h>
#include <float.h>
#include <stdbool.h>

#define PORT 8080
#define MAX_CONNECTIONS 300
#define FILE_SIZE 2097148
#define ID1 2781
#define ID2 8413

int recv_message(int);
int recv_half(int, int);

int numOfTimes = 0; //  Global variable for number of times saved.
int columns = 5; // Global variable representing number of columns in "times" array.
char buffer[FILE_SIZE + 1]; // Global array for holding the message. His size is FILE_SIZE + 1 for the \0.
double** times = NULL; // Global 2D array for holding times. First row is for first half, Second row is for second half.

int main() {
    //----------------------------------Create TCP Connection---------------------------------
    // Creates endpoint for communication named "socketFD". FD for file descriptor.
    int SocketFD = socket(AF_INET, SOCK_STREAM, 0);

    // Check if we were successful in creating socketFD.
    if (SocketFD == -1) {
        printf("(-) Could not create socket! -> socket() failed with error code: %d\n", errno);
        exit(EXIT_FAILURE); // Exit program and return EXIT_FAILURE (defined as 1 in stdlib.h).
    }
    else {
        printf("(=) Socket created successfully.\n");
    }

    // Check if address is already in use.
    int enableReuse = 1;
    if (setsockopt(SocketFD, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(enableReuse)) ==  -1)  {
        printf("setsockopt() failed with error code: %d\n" , errno);
        exit(EXIT_FAILURE); // Exit program and return EXIT_FAILURE (defined as 1 in stdlib.h).
    }

    // Create sockaddr_in for IPv4 for holding ip address and port and clean it.
    struct sockaddr_in serverAddress;
    memset(&serverAddress, '\0', sizeof(serverAddress));

    // Assign port and address to "serverAddress".
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT); // Short, network byte order.
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Binding port and address to socket and check if binding was successful.
    if (bind(SocketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        printf("(-) Failed to bind address && port to socket! -> bind() failed with error code: %d\n", errno);
        close(SocketFD); // close the socket.
        exit(EXIT_FAILURE); // Exit program and return EXIT_FAILURE (defined as 1 in stdlib.h).
    }
    else {
        printf("(=) Binding was successful!\n");
    }

    // Make server start listening and waiting, and check if listen() was successful.
    if (listen(SocketFD, MAX_CONNECTIONS) == -1) { // We allow no more than MAX_CONNECTIONS queue connections requests.
        printf("Failed to start listening! -> listen() failed with error code : %d\n", errno);
        close(SocketFD); // close the socket.
        exit(EXIT_FAILURE); // Exit program and return EXIT_FAILURE (defined as 1 in stdlib.h).
    }
    printf("(=) Waiting for incoming TCP-connections...\n");


    //----------------------------------Get TCP Connection From The Sender---------------------------------
    // Create sockaddr_in for IPv4 for holding ip address and port of client and cleans it.
    struct sockaddr_in clientAddress;
    memset(&clientAddress, 0, sizeof(clientAddress));
    unsigned int clientAddressLen = sizeof(clientAddress);
    int clientSocket = accept(SocketFD, (struct sockaddr*)&clientAddress, &clientAddressLen); // Accept connection.
    if (clientSocket == -1) {
        printf("(-) Failed to accept connection. -> accept() failed with error code: %d\n", errno);
        close(SocketFD);
        close(clientSocket);
        exit(EXIT_FAILURE); // Exit program and return EXIT_FAILURE (defined as 1 in stdlib.h).
    }
    else {
        printf("(=) Connection established.\n\n");
    }

    //----------------------------------Allocate Memory For Times Array---------------------------------
    times = (double**) malloc(2 * sizeof(double*));
    // Check if malloc was successful.
    if (times == NULL) {
        printf("(-) Error in allocating memory!");
    }

    // Allocate memory for every row.
    for (int i = 0; i < 2; i++) {
        times[i] = (double*) malloc(columns * sizeof(double));
        // Check if malloc was successful.
        if (times[i] == NULL) {
            printf("(-) Error in allocating memory!");
        }
    }

    // While the user still wants to send file, keep receiving.
    while(true) {
        if(recv_message(clientSocket) == -1){
            printf("\nExiting...\n\n");
            break;
        }
    }

    //----------------------------------Print Times ---------------------------------
    printf("Times in seconds for both halves:\n");
    for (int i = 0; i < numOfTimes; i++) {
        printf("%d - First Half: %.12f sec \t | \t Second Half: %.12f sec\t\n", i + 1, times[0][i], times[1][i]);
    }

    double sum1 = 0.0; // Variable to sum times for the first part.
    double sum2 = 0.0; // Variable to sum times for the second part.

    for (int i = 0; i < numOfTimes; i++) {
        sum1 += times[0][i];
        sum2 += times[1][i];
    }

    double average1 = sum1 / numOfTimes; // Average time for the first part.
    double average2 = sum2 / numOfTimes; // Average time for the second part.

    // Prints averages.
    printf("\nAverages:\n");
    printf("Average time for the first part: %lf\n", average1);
    printf("Average time for the second part: %lf\n", average2);
    printf("Average time for the whole file: %lf\n", (average1 + average2) / 2.0);

    //----------------------------------Free Allocated Memory---------------------------------
    for (int i = 0; i < 2; i++) {
        free(times[i]);
    }
    free(times);

    //----------------------------------Close Socket---------------------------------
    if(close(clientSocket) == -1) {
        printf("(-) Failed to close connection! -> close() failed with error code: %d\n", errno);
    }
    else {
        printf("(=) Connection closed!\n");
    }
}

//----------------------------------Receive Messages-----------------------------------------
// Method for receiving message from sender.
int recv_message(int clientSocket) {

    //----------------------------------Re-allocate Memory For Times Array---------------------------------
    // Will relocate only if necessary.
    if (numOfTimes >= columns) {
        columns += 5;
        for (int i = 0; i < 2; i++) {
            times[i] = (double *) realloc(times[i], sizeof(double) * columns);
            // Check if realloc() was successful.
            if (times[i] == NULL) {
                printf("(-) Error in re-allocating memory!");
            }
        }
    }

    //----------------------------------Change CC To Cubic-----------------------------------------------
    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_CONGESTION, "cubic", 5) == -1) {
        printf("(-) Failed to change cc algorithm! -> setsocketopt() failed with error code: %d", errno);
    }
    else {
        printf("(+) Set CC algorithm to: cubic");
    }

    //----------------------------------Receive First Messages---------------------------------
    if (recv_half(clientSocket, 0) == -1) {
        return -1;
    }
    printf("\n---------------------> Finished receiving first half <---------------------\n");
    bzero(buffer, FILE_SIZE + 1); // Clean buffer.

    //----------------------------------Send Authentication------------------------------------
    char xor[6] = {0}; // Array of holding the xor result. It's of size 5. +1 for the \0.
    sprintf(xor, "%d", ID1 ^ ID2); // Convert int (xor result) to string and put in "xor" array.
    if (send(clientSocket, xor, 5, 0) == -1) { // Send xor result to client.
        printf("(-) Failed to send authentication! -> send() failed with error code: %d\n", errno);
    }
    else {
        printf("(+) Sent authentication.\n");
    }

    //----------------------------------Change CC To Reno---------------------------------------------
    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_CONGESTION, "reno", 4) == -1) {
        printf("(-) Failed to change cc algorithm! -> setsocketopt() failed with error code: %d", errno);
    }
    else {
        printf("(+) Set CC algorithm to: reno");
    }

    //----------------------------------Receive Second Messages---------------------------------
    if(recv_half(clientSocket, 1) == -1) {
        return -1;
    }
    printf("\n---------------------> Finished receiving second half <---------------------\n\n");
    bzero(buffer, FILE_SIZE + 1); // Clean buffer.
    numOfTimes++; // Increase "numOfTimes" by 1.
    return 0;
}

int recv_half(int clientSocket, int row) {
    size_t receivedTotalBytes = 0; // Variable for keeping track of number of received bytes.
    double seconds; // Variable for measuring time in seconds.
    double microseconds; // Variable for measuring time in microseconds.
    double time; // Variable for measuring elapsed time.
    bool startTime = false; // Variable for knowing when to start timer.
    struct timeval begin, end; // Struct for measuring time.

    //----------------------------------Receive Half Loop---------------------------------
    while (receivedTotalBytes != FILE_SIZE / 2) {
        bzero(buffer, FILE_SIZE + 1); // Clean buffer.
        ssize_t receivedBytes = recv(clientSocket, buffer, FILE_SIZE / 2 - receivedTotalBytes, 0);
        if (receivedBytes <= 0) { // Break if we got an error (-1) or peer closed half side of the socket (0).
            printf("(-) Error in receiving data or peer closed half side of the socket.");
            break;
        }
        // Start time.
        if (startTime == false) {
            startTime = true;
            gettimeofday(&begin, 0);
        }
        receivedTotalBytes += receivedBytes; // Add the new received bytes to the total bytes received.

//        printf("%s", buffer);

        // Check if we need to exit.
        if (strcmp(buffer, "exit") == 0) {
            return -1;
        }
    }

    //----------------------------------Calculate Time---------------------------------
    gettimeofday(&end, 0); // End time.
    seconds = (double) (end.tv_sec - begin.tv_sec); // Calculate elapsed time in seconds.
    microseconds = (double) (end.tv_usec - begin.tv_usec); // Calculate elapsed time in microseconds.
    time = seconds + microseconds * (1e-6);
    times[row][numOfTimes] = time;

    return 0;
}