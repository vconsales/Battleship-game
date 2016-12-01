#include "TCP.h"

typedef enum { UNSET, NAME_SET, PEER_FREE, PEER_PLAYING } peer_state;

typedef struct Peer_t 
{
	ConnectionTCP conn;
	char name[64];
	uint16_t udp_port; /*porta(big endian) sulla quale il peer accetta connessioni da altri peer.*/
	peer_state state;
}Peer;


/***Variabili globali***/
ServerTCP *listener;
Peer**peers;
int max_peers = 50;
/**********************/

int main( int argc, char* argv[] )
{
	int sd, i, ret;
	uint16_t porta = 0;
	char buf[1024];

	if( argc < 2 ) {
		printf("Numero di porta non inserito!\n");
		exit(1);
	}

	size_t size_peer_str = sizeof(ConnectionTCP*) * max_peers;
	/*peers = (ConnectionTCP**)malloc(len);*/
	peers = (Peer**)malloc(size_peer_str);
	memset( peers, 0, size_peer_str ); /*azzero tutti i puntatori*/

	/*Ottengo porta*/
	sscanf( argv[1], "%hu", &porta );

	/**gestire errori**/
	ret = open_serverTCP( porta, &listener ); 
	if( ret == -1 )
		return -1;
	printf("Server aperto. Socket: %d \n",listener->socket);


	for( i=0 ; i<max_peers; i++ ) {
		peers[i] = (Peer*)malloc(sizeof(Peer));
		peers[i]->state = UNSET; 
		sd = accept_serverTCP( listener, &peers[i]->conn );
		 
		/*messaggio di benvenuto*/
		sprintf(buf,"WELCOME YOUR ID IS %d\n",i);		
		int n = send( sd, buf, 30, 0 );
		
		memset(buf,0,1024);

		//ricevo il nome 
		recv_data( sd, buf, 64);
		printf("%s\n",buf);
		sscanf( buf, "MY NAME IS %s",peers[i]->name); 
		//strcpy(peers[i]->name, buf);

		//controllare se esiste un altro con lo stesso nome
		sprintf(buf,"NAME SET");
		send(sd, buf, 18, 0);
		peers[i]->state = NAME_SET;

		//ricevo la porta su cui ascolta
		recv_data( sd, buf, 1024 );
		sscanf( buf, "MY PORT IS %hu",&peers[i]->udp_port); 
		peers[i]->state = PEER_FREE;

		printf("PEER %d NAME:%s UDP_PORT:%hu\n", i, peers[i]->name, ntohs(peers[i]->udp_port) );

	}

	close_serverTCP(&listener);
	return 0;
}
