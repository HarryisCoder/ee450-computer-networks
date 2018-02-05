/*
** This code is based on framework from Beej's guide 6.2 & 6.3 : http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "4144"  // the port users will be connecting to

#define FOLLOWER_NUM 5 // max number of followers

#define TWITTER_NUM 3 // max number of tweets

#define HEADERSIZE 10 // max number of bytes of header in message 

#define MAXDATASIZE 140 // max number of bytes we can send/get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// separate header from the received tweet
void cut_string(char* buf, char* header, char label) 
{
	int i = 0;
	while (buf[i] != label) {
		i++;
	}
	memcpy(header, &buf[0], i);
	header[i] = '\0';
	if (i == strlen(buf) - 1) {
		strcpy(buf, "");
	} else {
		char temp[MAXDATASIZE];
		memcpy(temp, &buf[i + 1], strlen(buf) - i - 1);
		temp[strlen(buf) - i - 1] = '\0';
		strcpy(buf, temp);
	}
}

int main(void)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXDATASIZE];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	// hardcoded follower ports
	const char *follower_port[FOLLOWER_NUM];
	follower_port[0] = "22244";
	follower_port[1] = "22344";
	follower_port[2] = "22444";
	follower_port[3] = "22544";
	follower_port[4] = "22644";

	// hardcoded follower names
	const char *follower[FOLLOWER_NUM];
	follower[0] = "Follower1";
	follower[1] = "Follower2";
	follower[2] = "Follower3";
	follower[3] = "Follower4";
	follower[4] = "Follower5";

	//--------------------------------receive tweets from server using UDP connection--------------------------------

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	int i = 0;
	// fork multiple client processes to receive tweets individually 
	for(i = 0; i < FOLLOWER_NUM; i++) {
    	int pid = fork();
    	if(pid < 0) {
	        printf("Error");
	        exit(1);
	    } else if (pid == 0) {
    		if ((rv = getaddrinfo("localhost", follower_port[i], &hints, &servinfo)) != 0) {
				fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
				return 1;
			}
			// loop through all the results and bind to the first we can
			for(p = servinfo; p != NULL; p = p->ai_next) {
				if ((sockfd = socket(p->ai_family, p->ai_socktype,
						p->ai_protocol)) == -1) {
					perror("listener: socket");
					continue;
				}

				if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
					close(sockfd);
					perror("listener: bind");
					continue;
				}

				break;
			}

			if (p == NULL) {
				fprintf(stderr, "listener: failed to bind socket\n");
				return 2;
			}

			freeaddrinfo(servinfo);

			// printf("[DEBUG]listener: waiting to recvfrom...\n");

			addr_len = sizeof their_addr;
			int bool_print_port = 1;

			while(1) {
				if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE-1 , 0,
					(struct sockaddr *)&their_addr, &addr_len)) == -1) {
					perror("recvfrom");
					exit(1);
				}

				if (bool_print_port == 1) {
					// get port & IP address after connection being built
					struct sockaddr_in client_addr;
					int client_addr_len;
					client_addr_len = sizeof(client_addr);

					if (getsockname(sockfd, (struct sockaddr *)&client_addr, &client_addr_len) == -1) {
						perror("getsockname() failed");
						return -1;
					}
					// print port & IP info.
					printf("%s has UDP port %d", follower[i], (int) ntohs(client_addr.sin_port));
					printf(" and IP address %s\n", inet_ntoa(client_addr.sin_addr));
					bool_print_port = 0;
				}
				// only receive message when message is not empty
				if (numbytes > 0) {
					buf[numbytes] = '\0';
					// when ending symbol received, stop receiving
					if (strcmp(buf, "#") == 0) {
						break;
					}
					char header[HEADERSIZE];
					cut_string(buf, header, '#');
					printf("%s has received %s\n", follower[i], header);
				}
			}
			printf("\n");
			close(sockfd);
			exit(0);
		} else {
			wait(NULL);
		}
	}
	sleep(1);


	//-------------------------read follower.txt and send feedback to server using TCP connection-------------------------

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo("localhost", PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	char str[MAXDATASIZE + HEADERSIZE];

	// fork multiple client processes to send feedback to the server individually 
	for(i = 0; i < FOLLOWER_NUM; i++) {
    	int pid = fork();
    	if(pid < 0) {
	        printf("Error");
	        exit(1);
	    } else if (pid == 0) {
			for(p = servinfo; p != NULL; p = p->ai_next) {
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

			// read feedback from input txt file
			char follower_file[HEADERSIZE];   //? why not char* follower_file?
			strcpy(follower_file, follower[i]);
			strcat(follower_file, ".txt");
			FILE* file = fopen(follower_file, "r"); 
			if(file == NULL) //if file does not exist, exit
			{
			    printf("No file found!");
			    exit(1);
			}
		    char line[MAXDATASIZE];
		    // ignore first two lines
		    fgets(line, sizeof(line), file);
		    fgets(line, sizeof(line), file);

			char temp[MAXDATASIZE];
			int j;
			for (j = 0; j < TWITTER_NUM; j++) {
				fgets(line, sizeof(line), file);

				if (strstr(line, "like") != NULL) {
					strcpy(temp, follower[i]);
					strcat(temp, "#Tweet");
					char* tweet_id;
				 	switch (j + 1) {
				 		case 1:
					 		strcpy(tweet_id, "A");
					 		break;
					 	case 2:
					 		strcpy(tweet_id, "B");
					 		break;
					 	case 3:
					 		strcpy(tweet_id, "C");
					 		break;
					 	default:
					 		strcpy(tweet_id, "#");
				 	}
					strcat(temp, tweet_id);
					if (send(sockfd, temp, strlen(temp), 0) == -1) {
						perror("send");
						exit(1);
					}
					// delay after sending one packet
					sleep(1);
				}
			}
			fclose(file);
			// once all feedbackd for a certain follower has been sent, send ending symbol to the server 
			strcpy(temp, follower[i]);
			strcat(temp, "#");
			if (send(sockfd, temp, strlen(temp), 0) == -1) {
				perror("send");
				exit(1);
			}
			
			// get port & IP address after connection being built
			struct sockaddr_in client_addr;
			int client_addr_len;
			client_addr_len = sizeof(client_addr);
			if (getsockname(sockfd, (struct sockaddr *)&client_addr, &client_addr_len) == -1) {
				perror("getsockname() failed");
				return -1;
			}
			// print port & IP info.
			printf("%s has TCP port %d", follower[i], (int) ntohs(client_addr.sin_port));
			printf(" and IP address %s\n", inet_ntoa(client_addr.sin_addr));

			printf("Completed sending feedback for %s.\n", follower[i]);

			printf("End of Phase 2 for %s\n\n", follower[i]);
			close(sockfd);
			exit(0);
		} else {
			wait(NULL);
		}
	}
	return 0;
}
