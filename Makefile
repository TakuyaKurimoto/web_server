CFLAGS := $(CFLAGS) -pthread -g -W -Wall -Wno-unused-parameter -I .
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

up:$(OBJS)
	$(CC) $(CFLAGS) -o main $(OBJS) picohttpparser/picohttpparser.c -ljansson; ./main;
$(OBJS):util.h

build:
		docker-compose build 
up-all:
		docker-compose up -d 
exec-takuya:
		docker exec -it takuya sh 
		
.PHONY: up
