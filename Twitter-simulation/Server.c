/*
** This code is based on framework from Beej's guide 6.1 & 6.3 : http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
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

#define PORT "4144"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

#define MAXDATASIZE 140 // max number of bytes we can send/get at once 

#define HEADERSIZE 10 // max number of bytes of header in message 

#define TWITTER_NUM 3 // max number of twitters

#define FOLLOWER_NUM 5 // max number of followers

void sigchld_handler(int s)
{
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


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
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	int numbytes;
	char buf[MAXDATASIZE];

	// hardcoded follower ports
	const char *follower_port[FOLLOWER_NUM];
	follower_port[0] = "22244";
	follower_port[1] = "22344";
	follower_port[2] = "22444";
	follower_port[3] = "22544";
	follower_port[4] = "22644";

	// hardcoded following relation map
	const int follow_table[TWITTER_NUM][FOLLOWER_NUM] = {{1,1,0,0,1}, {1,1,1,1,1}, {1,0,1,1,1}};

	// ====================================== phase 1 ==========================================

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo("localhost", PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		// create socket
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
		// bind socket
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	// get port & IP address of server
	struct sockaddr_in * server_addr = (struct sockaddr_in *)p->ai_addr;

	printf("The server has TCP port %d", (int) ntohs(server_addr->sin_port));
	printf(" and IP address %s\n", inet_ntoa(server_addr->sin_addr));

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	// listen on port
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	int tweeter_count = 0;
	FILE* phase2_done;

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		// accept connection from client process
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		// get connected client's IP address
		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
				if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
				    perror("recv");
				    exit(1);
				}
				char header[HEADERSIZE];
				// only receive message when message is not empty
				if (numbytes > 0) {
					buf[numbytes] = '\0';
					// write received message to cache file
					FILE* fp = fopen("tweet_cache.txt", "ab+");
					fprintf(fp, "%s\n", buf);
					fclose(fp);
					cut_string(buf, header, '#');
					printf("Received the tweet list from %s\n", header);
					char temp[HEADERSIZE];
					memcpy(temp, header, strlen(header) - 1);
					temp[strlen(header) - 1] = '\0';
				}

				int tweet_id = -1;
				if(strcmp(header, "TweetA") == 0) {
					tweet_id = 0;
				} else if (strcmp(header, "TweetB") == 0) {
					tweet_id = 1;
				} else if (strcmp(header, "TweetC") == 0) {
					tweet_id = 2;
				}
				// wait until the server received feedbacks from followers
				while(fopen("phase2_done.txt", "r") == NULL);
				// twitters begin receiving feedbacks from the server
				if (fopen("phase2_done.txt", "r") != NULL) {
					sleep(tweet_id * 10);
					int j;
					char temp[MAXDATASIZE];
					for (j = 0; j < FOLLOWER_NUM; j++) {
						if (follow_table[tweet_id][j] == 1) {
							switch (j + 1) {
								case 1 :
									strcpy(temp, "Follower1");
									break;
								case 2 :
									strcpy(temp, "Follower2");
									break;
								case 3 :
									strcpy(temp, "Follower3");
									break;
								case 4 :
									strcpy(temp, "Follower4");
									break;
								case 5 :
									strcpy(temp, "Follower5");
									break;
								default :
									tweet_id = -1;
							}
							strcat(temp, " is following ");
							strcat(temp, header);
					        if (send(new_fd, temp, strlen(temp), 0) == -1) {
								perror("send");
								exit(1);
							}
							sleep(1);
						}
					}

					char filename[MAXDATASIZE];
					strcpy(filename, header);
					strcat(filename, "_likelist.txt");
					FILE *file = fopen(filename, "r");
					char line[MAXDATASIZE];

				    while (fgets(line, sizeof(line), file)) {
				    	strtok(line, "\n");
				    	strcat(line, " has liked ");
				    	strcat(line, header);
				        if (send(new_fd, line, strlen(line), 0) == -1) {
							perror("send");
							exit(1);
						}
						sleep(1);
				    }
				    if (send(new_fd, "#", strlen("#"), 0) == -1) {
						perror("send");
						exit(1);
					}
					sleep(1);
				    printf("The server has sent feedback result to %s\n", header);
				    fclose(file);
				}	
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
		sleep(1);
		tweeter_count++;
		// Once received tweets from all twitters, phase 1 is over
		if (tweeter_count == TWITTER_NUM) {
			printf("End of Phase 1 for the server\n\n");
			break;
		}
	}
	close(sockfd);

	/// ====================================== phase 2 ==========================================

	//step 1: server send tweets to followers using UDP

	hints.ai_socktype = SOCK_DGRAM;

	int i = 0;
	for (i = 0; i < FOLLOWER_NUM; i++) {

		if ((rv = getaddrinfo("localhost", follower_port[i], &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return 1;
		}
		// loop through all the results and make a socket
		for(p = servinfo; p != NULL; p = p->ai_next) {
			if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
				perror("talker: socket");
				continue;
			}
			break;
		}

		if (p == NULL) {
			fprintf(stderr, "talker: failed to create socket\n");
			return 2;
		}

		struct sockaddr_in * server_addr = (struct sockaddr_in *)p->ai_addr;

		FILE* file = fopen("tweet_cache.txt", "r"); 
		if (file == NULL) {
	        printf("Error: file pointer is null.");
	        exit(1);
	    }
	    char line[MAXDATASIZE];
	    int j;
	    int bool_print_port = 1;

	    for (j = 0; j < TWITTER_NUM; j++) {
	    	fgets(line, sizeof(line), file);
	    	if (follow_table[j][i] == 1){
	    		if ((numbytes = sendto(sockfd, line, strlen(line), 0, p->ai_addr, p->ai_addrlen)) == -1) {
					perror("talker: sendto");
					exit(1);
				}
				// get and print port & IP info.
				if (bool_print_port == 1) {
			 		// get UDP port & IP address of server
					struct sockaddr_in server_addr_udp;
					int server_addr_len;
					server_addr_len = sizeof(server_addr_udp);
					if (getsockname(sockfd, (struct sockaddr *)&server_addr_udp, &server_addr_len) == -1) {
						perror("getsockname() failed");
						return -1;
					}
					printf("The server has UDP port %d", (int) ntohs(server_addr_udp.sin_port));
					// printf(" and IP address %s\n", inet_ntoa(server_addr.sin_addr));
					printf(" and IP address %s\n", inet_ntoa(server_addr->sin_addr));
					bool_print_port = 0;
			 	}
			 	char tweet_id;
			 	switch (j + 1) {
			 		case 1:
				 		tweet_id = 'A';
				 		break;
				 	case 2:
				 		tweet_id = 'B';
				 		break;
				 	case 3:
				 		tweet_id = 'C';
				 		break;
				 	default:
				 		tweet_id = '#';
			 	}
				printf("The server has sent tweet%c to Follower%u\n", tweet_id, i + 1);
				sleep(1);
	    	}
	    }
	    printf("\n");
	    if ((numbytes = sendto(sockfd, "#", strlen("#"), 0, p->ai_addr, p->ai_addrlen)) == -1) {
			perror("talker: sendto");
			exit(1);
		}
		sleep(1);

		freeaddrinfo(servinfo);

		close(sockfd);

	}

	//step 2: followers send feedback to server using TCP

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo("localhost", PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		// create socket
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
		// bind socket
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}

	// get port & IP address of server
	server_addr = (struct sockaddr_in *)p->ai_addr;

	printf("The server has TCP port %d", (int) ntohs(server_addr->sin_port));
	printf(" and IP address %s\n", inet_ntoa(server_addr->sin_addr));

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	// listen on port
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	int follower_count = 0;

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		// accept connection from client process
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}
		// get connected client's IP address
		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			while(1) {
				if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
				    perror("recv");
				    exit(1);
				}
				// only receive message when message is not empty
				if (numbytes > 0) {
					buf[numbytes] = '\0';
					char header[HEADERSIZE];
					cut_string(buf, header, '#');
					// once received sneding symbol, stop receiving
					if (strcmp(buf, "") == 0) {
						printf("Server received the feedback from %s\n", header);
						break;
					} else {
						// write received feedback to cache files
						strcat(buf, "_likelist.txt");
						FILE *fp = fopen(buf, "ab+");
						fprintf(fp, "%s\n", header);
					}
				}	
			}
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
		sleep(3);
		follower_count++;
		// Once receive feedbacks from all twitters, phase 1 is over
		if (follower_count == FOLLOWER_NUM) {
			break;
		}
	}
	close(sockfd);

	//step 3: server send feedbacks to twitters through TCP connection already built

	printf("\n");
	printf("The server has TCP port %d", (int) ntohs(server_addr->sin_port));
	printf(" and IP address %s\n", inet_ntoa(server_addr->sin_addr));

	FILE* phase2_done_signal = fopen("phase2_done.txt", "ab+");
	fclose(phase2_done_signal);

	while(wait(NULL)>0);

	printf("\nEnd of Phase2 for the server.\n");

	// delete all cache files
	remove("tweet_cache.txt");
	remove("phase2_done.txt");
	remove("TweetA_likelist.txt");
	remove("TweetB_likelist.txt");
	remove("TweetC_likelist.txt");
	// printf("Cache files deleted.\n");

	return 0;
}
