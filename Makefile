CC = gcc
CFLAGS = -Wall

SRCS = main.c address.c tlb.c simulation.c
OBJS = $(SRCS:.c=.o)

all: translate

translate: $(OBJS)
	$(CC) $(CFLAGS) -o translate $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f translate $(OBJS)

.PHONY: all clean
