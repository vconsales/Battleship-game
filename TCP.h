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

#define NO_MEMORY 10
#define N_BYTE_NOT_DEFINED 11
#define LESS_BYTE_RECEIVED 12

typedef struct ServerTCP_t
{
	int socket;
	struct sockaddr_in my_addr;
	int peers_connected;	
}ServerTCP;

typedef struct ConnectionTCP_t
{
	int socket;
	struct sockaddr_in cl_addr; 
}ConnectionTCP;


int my_errno;

/**Richiede numero di porta little endian**/
int open_serverTCP( uint16_t port, ServerTCP **serv );

int close_serverTCP( ServerTCP **serv );

/**restituisce id della socket che ha avuto la connessione*/
int accept_serverTCP( ServerTCP *serv, ConnectionTCP *conn ); 

/*riceve al massimo buf_len byte e li immette in buf.
 *la funzione negozia automaticamente il numero di byte
 *da scambiare.
 */
int recv_data( int sockt, char* buf, uint32_t buf_len );

/*l'ultimo parametro dice quanti byte inviare*/
int send_data( int sockt, char* buf, uint32_t buf_len );
/*int recv_command();*/
int close_connection( ConnectionTCP *conn );
#endif
