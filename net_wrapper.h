#ifndef TCP_H
#define TCP_H	

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

typedef struct ConnectionTCP_t
{
	int socket;
	struct sockaddr_in cl_addr; 
}ConnectionTCP;

int my_errno;

/**Richiede numero di porta little endian**/
int open_serverTCP( uint16_t port );

/**restituisce id della socket che ha avuto la connessione*/
int accept_serverTCP( int sock_serv, ConnectionTCP *conn ); 

/*
* Riceve dati dalla socket indicata. La funzione negozia
* il numero di byte da ricevere ed alloca un buffer in cui
* depositare il messaggio. Il buffer verra' allocato nel
* puntatore passato come parametro.
*/
int recv_data( int sockt, char** buf );

/*l'ultimo parametro dice quanti byte inviare*/
int send_data( int sockt, char* buf, uint32_t buf_len );

/*int recv_command();*/
int close_connection( ConnectionTCP *conn );
#endif
