SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

up:$(OBJS)
	$(CC) -o main $(OBJS); ./main;
$(OBJS):util.h
.PHONY: up