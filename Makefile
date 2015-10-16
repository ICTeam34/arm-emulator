CC      = gcc
CFLAGS  = -Wall -g -D_POSIX_SOURCE -D_BSD_SOURCE -std=c99 -Werror -pedantic 
LIBS    = $(shell sdl-config --cflags --libs)

.SUFFIXES: .c .o

.PHONY: all clean

all: emulate

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

%.o: %.c
	$(CC) -c -o $*.o $(CFLAGS) $*.c

emulate: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)
	
clean:
	rm -f $(wildcard *.o)
	rm -f emulate
