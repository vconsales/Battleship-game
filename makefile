CC=gcc
CFLAGS= -Wall 

all: battle_server battle_client

battle_client: battle_client.o game.o TCP.o
	$(CC) $(CFLAGS) -o $@ battle_client.o game.o TCP.o
battle_server: battle_server.o TCP.o
	$(CC) $(CFLAGS) -o $@ battle_server.o TCP.o

battle_client.o: battle_client.c
	$(CC) $(CFLAGS) -o $@ -c battle_client.c

battle_server.o: battle_server.c
	$(CC) $(CFLAGS) -o $@ -c battle_server.c

TCP.o: TCP.c
	$(CC) $(CFLAGS) -o $@ -c TCP.c 
game.o: game.c
	$(CC) $(CFLAGS) -o $@ -c game.c

clean: 
	rm -f *.o battle_server battle_client
