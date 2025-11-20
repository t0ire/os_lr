CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=200112L
LDFLAGS=-lrt

all: parent child

parent: parent.c os_linux.c
	$(CC) $(CFLAGS) -o parent parent.c os_linux.c $(LDFLAGS)

child: child.c os_linux.c
	$(CC) $(CFLAGS) -o child child.c os_linux.c $(LDFLAGS)

clean:
	rm -f parent child

.PHONY: all clean