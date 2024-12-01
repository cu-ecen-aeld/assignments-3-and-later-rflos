CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror -g
LDFLAGS ?= -pthread -lrt
TARGET ?= aesdsocket

all: $(TARGET)

aesdsockt: aesdsocket.c
	$(CC) $(CFLAGS) $^ -o $@ $(INCLUDES) ${LDFLAGS}

.PHONY: clean
clean:
	rm -f aesdsocket
