/* 
 * File:   sender_main.c
 * Author: 
 *
 * Created on 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/time.h>

// The self-implemented tcp over udp protocol
#include "tcpoverudp.h"

// The utility functions
#include "util.h"

struct sockaddr_in si_other;
int s, slen;

void diep(char *s) {
    perror(s);
    exit(1);
}

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    //Open the file
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }

	/* Determine how many bytes to transfer */
    // unsigned long long int fileSize;
    // struct stat fileStats;
    // if (fstat(fileno(fp), &fileStats) == -1) {
    //     printf("Could not determine file size.\n");
    //     exit(1);
    // }
    // fileSize = fileStats.st_size;
    // bytesToTransfer = (fileSize < bytesToTransfer) ? fileSize : bytesToTransfer;

    // Initialize the server address structure
    slen = sizeof (si_other);
    memset((char *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(hostUDPport);
    if (inet_aton(hostname, &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    // Create UDP socket
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    // Bind the socket to any valid IP address and a specific port
    struct sockaddr_in si_me;
    int port = 0;
    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s, (struct sockaddr *) &si_me, sizeof (si_me)) == -1)
        diep("bind");

	/* Send data and receive acknowledgements on s*/
    // Initialize the connection
    TOUConnection *connection = touConnect(s, si_other, slen);
    if (connection == NULL) {
        printf("Could not establish connection.\n");
        exit(1);
    }

    // Send the file
    char buffer[SEND_BUFFER_SIZE];
    unsigned long long int bytesRead;
    unsigned long long int bytesSent = 0;
    while (bytesSent < bytesToTransfer) {
        unsigned long long int temp = SEND_BUFFER_SIZE > bytesToTransfer - bytesSent ? bytesToTransfer - bytesSent : SEND_BUFFER_SIZE;
        bytesRead = fread(buffer, 1, temp, fp);
        if (touSend(connection, buffer, bytesRead) == -1) {
            printf("Could not send data.\n");
            exit(1);
        }
        bytesSent += bytesRead;
    }
    printf("Sent %llu bytes.\n", bytesSent);

    // Close the file
    printf("Closing the file\n");
    fclose(fp);

    // Close the connection
    printf("Closing the connection\n");
    touClose(connection);

    // Free the connection
    printf("Freeing the connection\n");
    touFree(connection);

    // Close the socket
    printf("Closing the socket\n");
    close(s);
    return;

}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);

    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);

    return (EXIT_SUCCESS);
}


