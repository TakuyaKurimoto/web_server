SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

up:$(OBJS)
	gcc -o main $(OBJS); ./main;
$(OBJS):util.h
.PHONY: up