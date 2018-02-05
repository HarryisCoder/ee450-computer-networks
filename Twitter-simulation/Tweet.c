/*
** This code is based on framework from Beej's guide 6.2 : http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
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

#define PORT "4144" // the port client will be connecting to 

#define MAXDATASIZE 140 // max number of bytes we can send/get at once 

#define HEADERSIZE 10 // the max number of chars of header

#define TWITTER_NUM 3 // the number of twitters

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// int main(int argc, char *argv[])
int main(void)
{
	int sockfd, numbytes; 
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	// hardcoded twitter names
	const char *twitter[TWITTER_NUM];
	twitter[0] = "TweetA";
	twitter[1] = "TweetB";
	twitter[2] = "TweetC";

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo("localhost", PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	char str[MAXDATASIZE + HEADERSIZE];
	int i;
	// fork multiple client processes to send tweets individually 
	for(i = 0; i < TWITTER_NUM; i++) {
    	int pid = fork();
    	if(pid < 0) {
	        printf("Error");
	        exit(1);
	    } else if (pid == 0) {
		// loop through all the results and connect to the first we can
			for(p = servinfo; p != NULL; p = p->ai_next) {
				// create socket
				if ((sockfd = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
					perror("client: socket");
					continue;
				}
				// connect to server
				if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
					perror("client: connect");
					close(sockfd);
					continue;
				}

				break;
			}

			if (p == NULL) {
				fprintf(stderr, "client: failed to connect\n");
				return 2;
			}

			freeaddrinfo(servinfo); // all done with this structure

			char header_tweet[HEADERSIZE];
			strcpy(header_tweet, twitter[i]);
			char file_tweet[HEADERSIZE + 4];
			strcpy(file_tweet, header_tweet);
			strcat(file_tweet, ".txt");
			strcpy(str, header_tweet);

			// read tweet message from input txt file
			FILE* file = fopen(file_tweet, "r"); 
		    char line[MAXDATASIZE];
		    fgets(line, sizeof(line), file);
		    strcat(str, "#");
		    // add header to the message
			strcat(str, line);

			// send tweet to server
			if (send(sockfd, str, strlen(str), 0) == -1) {
				perror("send");
				exit(1);
			}
			
			struct sockaddr_in client_addr;
			int client_addr_len;
			client_addr_len = sizeof(client_addr);

			// get port & IP address after connection being built
			if (getsockname(sockfd, (struct sockaddr *)&client_addr, &client_addr_len) == -1) {
				perror("getsockname() failed");
				return -1;
			}
			printf("%s has TCP port %d", header_tweet, (int) ntohs(client_addr.sin_port));
			printf(" and IP address %s for Phase 1\n", inet_ntoa(client_addr.sin_addr));

			printf("%s is now connected to the server\n", header_tweet);

			printf("%s has sent \"%s\" to the server\n", header_tweet, line);
			printf("Updating the server is done for %s\n", header_tweet);
			printf("End of Phase 1 for %s\n\n", header_tweet);

			// receiving feedback from the server
			int bool_print_port = 1;
			while (1) {
				// waiting for feedback from the server
				if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
				    perror("recv");
				    exit(1);
				}
				buf[numbytes] = '\0';
				// when ending symbol received, stop receiving
				if (strcmp(buf, "#") == 0) {
					break;
				}
				// only receive message when message is not empty
				if (numbytes > 0) {
					if (bool_print_port == 1) {
						// print port & IP info.
						printf("%s has TCP port %d", header_tweet, (int) ntohs(client_addr.sin_port));
						printf(" and IP address %s for Phase 2\n", inet_ntoa(client_addr.sin_addr));
						bool_print_port = 0;
					}
					buf[numbytes] = '\0';
					// print reveived message
					printf("%s\n", buf);
				}
			}
			printf("End of Phase2 for %s\n\n", twitter[i]);
			close(sockfd);
			exit(0);
		} else {
			sleep(1);
		}
	}
	while(wait(NULL)>0);
	return 0;
}
