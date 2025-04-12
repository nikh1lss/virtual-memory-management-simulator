CC = gcc
CFLAGS = -Wall

all: translate

translate: translate.c
	$(CC) $(CFLAGS) -o translate translate.c

clean:
	rm -f translate *.o

.PHONY: all clean
