CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror -pthread -lrt -g

all: aesdsocket

aesdsockt: aesdsocket.c
	$(CC) $(CFLAGS) aesdsocket.c -o aesdsocket

.PHONY: clean
clean:
	rm -f aesdsocket
