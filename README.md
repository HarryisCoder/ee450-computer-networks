## EE450 Computer Networks
#### Project: Simualtion of Twitter process

#### 1. What I have done?

In this assignment I have written a program in c to simulate a twitter process. In phase 1, three different twitters will send different messages to the server, and the server will receive all messages from twitters and print out acknowledgement messages. In phase 2, server stored tweets from twitters first and then sent them to each twitter's followers. After receiving tweets from the server, followers sent feedbacks back to the server. Server stored these feedbacks and finally sent them to twitters.

#### 2. About code files

There are three code files in total: Server.c, Tweet.c and Follower.c.
In Server.c, first, the server process created TCP socket to connect to twitters. Second, the server created UDP socket to send tweets to followers. Then, the server create TCP socket to receive feedbaks from followers. Finally, server uses TCP socket built before to send feedbacks to twitters. 
In Tweet.c, three client(tweet) processes are created using fork(), each process will create TCP socket to connect to, send message and finally receive feedbacks from the server.
In Follwer.c, five client(follower) processes are created using fork(), each process will first create UDP socket to receive feedbacks from the server. Then each process will create TDP socket to send feedbacks to the server.

#### 3. How to run.

Compiling step: just run "make all" and you will get three output files: server, tweet and follower.
Running step: open three terminals in the same directory. First, in terminal 1 run "./server". Second, in terminal 2 run "./follower". Third, in terminal 3 run "./tweet"
Note: cache files will be created during process, but they will be deleted automatically when the whole process is finished.

#### 4. Reused code.

Server.c, Tweet.c and Follower.c are all based on the code framework from Beej's guide chap.6 : http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html