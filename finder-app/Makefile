CROSS_COMPILE ?=

CC = $(if $(CROSS_COMPILE),$(CROSS_COMPILE)gcc,gcc)
LD = $(if $(CROSS_COMPILE),$(CROSS_COMPILE)ld,ld)
AS = $(if $(CROSS_COMPILE),$(CROSS_COMPILE)as,as)

build: writer.o
	$(CC) -o writer writer.o
writer.o: writer.c
	$(CC) -c writer.c
clean:
	rm -rf *.o writer
default: build
