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
ServerTCP* listener;
Peer** peers;
int max_peers = 50;
const unsigned int DIM_BUF = 1024;
/**********************/

int analyze_message( int sockt, char* buf, size_t max_len );
int get_peer( int sockt, Peer** p );
int send_welcome(int sockt, int id );

int main( int argc, char* argv[] )
{
	int sd, i=0, j=0, ret=0;
	uint16_t porta = 0;
	char buf[DIM_BUF];
//	size_t len;
	int fdmax;

	fd_set master; /*set principale*/
	fd_set read_fds; /*set di lettura*/
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

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


	/*aggiungo listener nel set*/
	FD_SET( listener->socket, &master ); 
	fdmax = listener->socket;	

	while( i < max_peers )
	{
		read_fds = master;
		select( fdmax+1, &read_fds, NULL, NULL, NULL);

		/*scorro i descrittori*/
		for( j=0; j<=fdmax; j++ )
		{
			if( FD_ISSET(j, &read_fds) )
			{
				if( j == listener->socket ){
				//	ret = listen(listener, 10);	 //forse non serve				
					peers[i] = (Peer*)malloc(sizeof(Peer));
					peers[i]->state = UNSET; 
					sd = accept_serverTCP( listener, &peers[i]->conn );
					send_welcome(sd, i++);

					FD_SET( sd, &master );
					if( sd > fdmax )
						fdmax = sd;		
				} else {
					ret = recv_data(j, buf, DIM_BUF);
					if( ret == -1 )
						close(j);
					else {
						/*Il messaggio ricevuto viene analizzato secondo
						 *il protocollo scelto. Se necessario vengono
						 *aggiornate le strutture dati associate ai peer*/
						ret = analyze_message(j, buf, DIM_BUF);
						if( ret == -1 )
							printf("Messaggio non riconosciuto\n");
					}
				}
			}
		

			
			 
			// //messaggio di benvenuto
			// 
			
			// memset(buf,0,1024);


			// printf("PEER %d NAME:%s UDP_PORT:%hu\n", i, peers[i]->name, ntohs(peers[i]->udp_port) );
		}
	}

	close_serverTCP(&listener);
	return 0;
}

int analyze_message( int sockt, char* buf, size_t max_len )
{
	Peer* p;
	char i_buf[DIM_BUF];
	int res = -1;

	if( get_peer(sockt, &p) )
	{
		//gestire gli errori
		switch( p->state )
		{
			case UNSET:
				sscanf(buf, " MY NAME IS %s",p->name); 

				printf("Si e' connesso %s\n",p->name);
				//controllare se esiste un altro con lo stesso nome
				sprintf(i_buf, "NAME SET");
				send_data(sockt, i_buf, DIM_BUF);
				p->state = NAME_SET;
				res = 0;
				break;
			case NAME_SET:
				//ricevo la porta su cui ascolta
				sscanf(buf, "MY PORT IS %hu", &p->udp_port); 
			   	p->state = PEER_FREE;
			   	res = 1;
			   	printf("PEER %s riceve su porta UDP %hu \n",p->name,ntohs(p->udp_port));
				break;
			case PEER_FREE:
				res = 3;
				break;
			case PEER_PLAYING:
				res = 4;
				break;
			default:
				res = -1;
		}
		return res;
	}
	else{
		printf("Peer non trovato\n");
		return res;
	}
}

int send_welcome( int sockt, int id )
{
	/*invia il messaggio di benvenuto con l'id*/
	char buf[100];
	int res;
	sprintf(buf,"WELCOME YOUR ID IS %d",id);
	//printf("Mando: %s\n",buf);
	res = send_data(sockt, buf, 100);		
	// int n = send( sd, buf, 30, 0 );
	return res;
}

int get_peer( int sockt, Peer** p )
{
	int i, found = 0;
	for( i=0; i<listener->peers_connected; i++)
	{
		if( peers[i]->conn.socket == sockt ){
			*p = peers[i];
			found = 1;
			break;
		}
	}
	return found;
}