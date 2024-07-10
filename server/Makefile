CROSS_COMPILE ?=
CFLAGS = -Wall -Werror -g

CC = $(if $(CROSS_COMPILE),$(CROSS_COMPILE)gcc,gcc)

build: aesdsocket.o
	$(CC) -o aesdsocket $(CFLAGS) aesdsocket.o
writer.o: writer.c
	$(CC) -c aesdsocket.c
clean:
	rm -rf *.o aesdsocket
all: build
default: all
