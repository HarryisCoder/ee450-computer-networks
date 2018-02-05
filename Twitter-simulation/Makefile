# Makefile for EE 450 project phase 2

all: server tweet follower

server: Server.c
	gcc -o server Server.c -lsocket -lnsl -lresolv

tweet: Tweet.c
	gcc -o tweet Tweet.c -lsocket -lnsl -lresolv

follower: Follower.c
	gcc -o follower Follower.c -lsocket -lnsl -lresolv
	