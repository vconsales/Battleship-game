CC=gcc
CFLAGS= -Wall -g

all: battle_server battle_client

battle_client: battle_client.o game.o TCP.o
	$(CC) $(CFLAGS) -o $@ battle_client.o game.o TCP.o

battle_server: battle_server.o TCP.o peer_manager.o
	$(CC) $(CFLAGS) -o $@ battle_server.o TCP.o peer_manager.o

battle_client.o: battle_client.c
	$(CC) $(CFLAGS) -o $@ -c battle_client.c

battle_server.o: battle_server.c 
	$(CC) $(CFLAGS) -o $@ -c battle_server.c 

TCP.o: TCP.c
	$(CC) $(CFLAGS) -o $@ -c TCP.c 

peer_manager.o: peer_manager.c
	$(CC) $(CFLAGS) -o $@ -c peer_manager.c 

game.o: game.c
	$(CC) $(CFLAGS) -o $@ -c game.c

peer_manager.c: peer_manager.h
TCP.c: TCP.h	
game.c: game.h

clean: 
	rm -f *.o battle_server battle_client
