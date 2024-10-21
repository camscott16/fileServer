CC = gcc
CFLAGS = -Wall -g

SERVER_OBJS = server.o TCPServerUtility.o DieWithMessage.o AddressUtility.o
CLIENT_OBJS = client.o TCPClientUtility.o DieWithMessage.o AddressUtility.o

all: server client

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o server $(SERVER_OBJS)

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o client $(CLIENT_OBJS)

server.o: server.c Practical.h
	$(CC) $(CFLAGS) -c server.c

client.o: client.c Practical.h
	$(CC) $(CFLAGS) -c client.c

TCPServerUtility.o: TCPServerUtility.c Practical.h
	$(CC) $(CFLAGS) -c TCPServerUtility.c

TCPClientUtility.o: TCPClientUtility.c Practical.h
	$(CC) $(CFLAGS) -c TCPClientUtility.c

DieWithMessage.o: DieWithMessage.c Practical.h
	$(CC) $(CFLAGS) -c DieWithMessage.c

AddressUtility.o: AddressUtility.c Practical.h
	$(CC) $(CFLAGS) -c AddressUtility.c

clean:
	rm -f *.o server client

