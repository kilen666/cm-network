/* 
 * File:   receiver_main.c
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

// The self-implemented tcp over udp protocol
#include "tcpoverudp.h"

// The utility functions
#include "util.h"

struct sockaddr_in si_me, si_other;
int s, slen;

void diep(char *s) {
    perror(s);
    exit(1);
}

void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    // Open the file
    FILE *fp;
    fp = fopen(destinationFile, "wb");
    if (fp == NULL) {
        printf("Could not open file to write.");
        exit(1);
    }

    // Prepare socket address structure
    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    // Create a UDP socket
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    // Bind the socket to any valid IP address and a specific port
    slen = sizeof (si_other);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");

    /* Now receive data and send acknowledgements */ 
    // Establish connection
    TOUConnection *connection = touAccept(s);
    if (connection == NULL) {
        printf("Could not establish connection.\n");
        exit(1);
    }

    // Receive the file
    char buffer[RECV_BUFFER_SIZE];
    int bytesRead;
    unsigned long long int bytesReceived = 0;
    while ((bytesRead = touRecv(connection, buffer, RECV_BUFFER_SIZE)) >= 0) {
        fwrite(buffer, 1, bytesRead, fp);
        bytesReceived += bytesRead;
    }
    printf("Received %llu bytes.\n", bytesReceived);

    // Close the file
    fclose(fp);

    // Close the connection
    touClose(connection);

    // Free the connection
    touFree(connection);   

    // Close the socket
    close(s);
	printf("%s received.\n", destinationFile);
    return;
}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}

