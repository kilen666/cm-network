/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "80" // the port client will be connecting to 

#define MAXDATASIZE 128 // max number of bytes we can get at once 

#define USAGE "usage: http_client http://hostname[:port]/path/to/file\n"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char parse_url(char *url, char *hostname, char *port, char *path) {
	// Check if URL is valid
	if (url == NULL) {
		return 0;
	}
	// Parse URL
	int rv;
	if ((rv = sscanf(url, "http://%99[^:/]:%9[^/]", hostname, port)) == 0 || *hostname == '\0') {
		return 0;
	}
	else if (rv == 1) {
		strcpy(port, PORT);
		sscanf(url, "http://%99[^:/]/%99[^\n]", hostname, path);
	}
	else {
		sscanf(url, "http://%99[^:/]:%9[^/]/%99[^\n]", hostname, port, path);
	}
	return 1;
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	// Check arguments
	if (argc != 2) {
	    fprintf(stderr, USAGE);
	    exit(1);
	}

	// Initialize addrinfo
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// Parse URL
	char *url = argv[1];
	char hostname[100] = {0};
	char path[100] = {0};
	char port[10] = {0};

	if (!parse_url(url, hostname, port, path)) {
		fprintf(stderr, USAGE);
		return 1;
	}


	// printf("Hostname: %s\n", hostname);
	// printf("Port: %s\n", port);
	// printf("Path: %s\n", path);

	// Get address info
	if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	// Print connection info
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	// Send request
	char request[500];
	snprintf(request, sizeof(request), "GET /%s HTTP/1.1\r\nHost: %s:%s\r\nConnection: close\r\n\r\n", path, hostname, port);
	if (send(sockfd, request, strlen(request), 0) == -1) {
	    perror("client: send");
	    return 3;
	}

	// Prepare output file
	FILE *output = fopen("output", "w");
	if (!output) {
	    perror("client: fopen");
	    return 4;
	}

	// Receive response
	char isHeader = 1;
	printf("\n");
	while ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) > 0) {
	    buf[numbytes] = '\0';
		// Check if header is finished
		if (isHeader) {
			char *endHeader = strstr(buf, "\r\n\r\n");
			if (endHeader) {
				isHeader = 0;
				printf("%.*s\n", (int)(endHeader - buf), buf);
				fwrite(endHeader + 4, 1, numbytes - (endHeader - buf) - 4, output);
			}
			else {
				printf("%s", buf);
			}
		}
		else {
			fwrite(buf, 1, numbytes, output);
		}
	}

	// Receive error handling
	if (numbytes == -1) {
	    perror("client: recv");
	    exit(5);
	}

	// Close output file and socket
	fclose(output);
	close(sockfd);

	return 0;
}