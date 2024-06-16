all: writer

writer.o: writer.c
	$(CC) $(CCFLAGS) -c main.c

writer: writer.o
	$(CC) $(LDFLAGS) writer.o -o writer
clean:
	rm writer.o writer
