CC = gcc


all: client server

client:
	$(CC) -o c ./src/client.c

server:
	$(CC) -o s ./src/server.c

clean:
	rm -f $(OBJS) $(EXEC)
