#include "TCP.h"
#include "peer_manager.h"


/***Variabili globali***/
ServerTCP* listener;
extern des_peer** peers;
extern int max_peers;
const unsigned int DIM_BUF = 1024;
/**********************/

int analyze_message( int sockt, char* buf, size_t max_len );
int send_welcome(int sockt, int id );

int main( int argc, char* argv[] )
{
	int sd, i=0, j=0, ret=0;
	uint16_t porta = 0;
	char buf[DIM_BUF];
	int fdmax;

	fd_set master; /*set principale*/
	fd_set read_fds; /*set di lettura*/
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	if( argc < 2 ) {
		printf("Numero di porta non inserito!\n");
		exit(1);
	}

	size_t size_peer = sizeof(des_peer*) * max_peers;
	peers = (des_peer**)malloc(size_peer);
	memset( peers, 0, size_peer ); /*azzero tutti i puntatori*/

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

	while( listener->peers_connected < get_max_peers() )
	{
		read_fds = master;
		select( fdmax+1, &read_fds, NULL, NULL, NULL);

		/*scorro i descrittori*/
		for( j=0; j<=fdmax; j++ )
		{
			if( FD_ISSET(j, &read_fds) )
			{
				if( j == listener->socket ){
					//ret = listen(listener->socket, 10);	 //forse non serve				
					i = add_peer(listener->peers_connected);
					if( i != -1 ){
						sd = accept_serverTCP( listener, &peers[i]->conn );
						send_welcome(sd, i);

						FD_SET( sd, &master );
						if( sd > fdmax )
							fdmax = sd;
					} else
						printf("Connessione rifiutata. Troppi peer connessi\n");		
				} else {
					//printf("--socket non listener\n");
					ret = recv_data(j, buf, DIM_BUF);
					if( ret == -1 ){
						close(j);
						//printf("chiudo il socket %d \n",j);
						FD_CLR(j,&master);
						remove_peer_having_sock(j);
						/*ridurre fdmax. test*/
						if( j == fdmax)
							fdmax--;

						listener->peers_connected--;
					}
					else {
						/*Il messaggio ricevuto viene analizzato secondo
						 *il protocollo scelto. Se necessario vengono
						 *aggiornate le strutture dati associate ai peer*/
						//printf("Ricevuto messaggio\n");
						ret = analyze_message(j, buf, DIM_BUF);
						if( ret == -1 )
							printf("Messaggio non riconosciuto\n");
					}
				}
			}
		}
	}

	close_serverTCP(&listener);
	return 0;
}

int analyze_message( int sockt, char* buf, size_t max_len )
{
	des_peer* p;
	char i_buf[DIM_BUF];
	int res = -1;
	int index = get_index_peer(sockt);

	if( index != -1 )
	{
		p = peers[index];
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
				printf("PEER FREE\n");
				res = 3;
				break;
			case PEER_PLAYING:
				printf("PEER PLAYING\n");
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

