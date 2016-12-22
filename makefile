CC=gcc
CFLAGS= -Wall

all: battle_server battle_client

battle_client: battle_client.o game.o net_wrapper.o
	$(CC) $(CFLAGS) -o $@ battle_client.o game.o net_wrapper.o

battle_server: battle_server.o net_wrapper.o peer_manager.o
	$(CC) $(CFLAGS) -o $@ battle_server.o net_wrapper.o peer_manager.o

battle_client.o: battle_client.c
	$(CC) $(CFLAGS) -o $@ -c battle_client.c

battle_server.o: battle_server.c 
	$(CC) $(CFLAGS) -o $@ -c battle_server.c 

net_wrapper.o: net_wrapper.c
	$(CC) $(CFLAGS) -o $@ -c net_wrapper.c 

peer_manager.o: peer_manager.c
	$(CC) $(CFLAGS) -o $@ -c peer_manager.c 

game.o: game.c
	$(CC) $(CFLAGS) -o $@ -c game.c

peer_manager.c: peer_manager.h
net_wrapper.c: net_wrapper.h	
game.c: game.h

clean: 
	rm -f *.o battle_server battle_client
