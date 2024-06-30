CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror

all: aesdsocket

aesdsockt: aesdsocket.c
	$(CC) $(CFLAGS) aesdsocket.c -o aesdsocket

.PHONY: clean
clean:
	rm -f aesdsocket
