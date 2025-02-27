/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler()
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
//to handle request
void request__dealing(int newfd)
{
    char buffer[1024];
    int bytesnum;
    //to read http_request
    if((bytesnum=recv(newfd,buffer, sizeof(buffer)-1, 0))==-1)    {
        perror("get wrong in recv");
        close(newfd);
        return;
    }
    buffer[bytesnum]='\0';
    printf("received_request:\n%s\n", buffer);

    //to parse the request
    char method[16], path[256], version[16];
    sscanf(buffer, "%s %s %s", method, path, version);

    //establish a connection
    FILE *file=fopen(path+1,"r");
    if(file==NULL){//if file not found
        const char word404[50]="HTTP/1.1 404 Not Found\r\n\r\n";
        send(newfd, word404, strlen(word404), 0);
    }
    else{//found
        const char word200[20]="HTTP/1.1 200 OK\r\n\r\n";
        send(newfd, word200, strlen(word200), 0);
        char file_get[1024];
        while(fgets(file_get, sizeof(file_get), file)!=NULL){
            send(newfd, file_get, strlen(file_get), 0);
        }
        fclose(file);
    }

}
int main(int argc, char *argv[])
{
	int sockfd, newfd;  // listen on sock_fd, new connection on newfd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
    if(argc != 2)
    {
        fprintf(stderr, "usage: http_server port\n");
    }

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		newfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (newfd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			request__dealing(newfd);

			exit(0);
		}
		close(newfd);  // parent doesn't need this
	}

	return 0;
}

