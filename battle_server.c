#include "TCP.h"
#include "peer_manager.h"
#include "messages.h"

/***Variabili globali***/
ServerTCP* listener;
extern des_peer** peers;
extern int max_peers;
const unsigned int DIM_BUF = 1024;
/**********************/

int analyze_message( int sockt, char* buf, size_t max_len );
int send_welcome( int sockt, int id );
int send_list_of_peer( int sockt );
int connect_request( int sender_id, char* opponent_name );

int main( int argc, char* argv[] )
{
	int sd, i=0, j=0, ret=0;
	uint16_t porta = 0;
	char* buf_rec = NULL;
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

	//while( listener->peers_connected < get_max_peers() )
	while(1)
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
					ret = recv_data(j, &buf_rec);
					//printf("--mess: %s\n",buf);
					if( ret == -1 ){
						close(j);
						printf("chiudo il socket %d \n",j);
						FD_CLR(j,&master);
						remove_peer_having_sock(j);
						/*ridurre fdmax. test*/
						if( j == fdmax)
							fdmax--;

						listener->peers_connected--;
					} else {
						/*Il messaggio ricevuto viene analizzato secondo
						 *il protocollo scelto. Se necessario vengono
						 *aggiornate le strutture dati associate ai peer*/
						//printf("Ricevuto messaggio\n");
						ret = analyze_message(j, buf_rec, DIM_BUF);
						if( ret == -1 )
							printf("Messaggio non riconosciuto\n");
					}
				}
			}
		}
	}

	close_serverTCP(&listener);
	free(buf_rec);

	return 0;
}

int analyze_message( int sockt, char* buf, size_t max_len )
{
	des_peer* p;
	char cmd[50];
	char arg[100];
	int res = -1;
	message_type m;

	//printf("analizzo messaggio\n");
	int index = get_index_peer_sock(sockt);

	if( index == -1 ) {
		printf("Peer non trovato\n");
		return res;
	}

	p = peers[index];
	//printf("Lo stato del peer %d e' %d\n",index,peers[index]->state);

	//gestire gli errori
	switch( p->state )
	{
		case UNSET:
			//sscanf(buf, " MY NAME IS %s",p->name); 
			memcpy(&m, buf, sizeof(m));
			m = ntohl(m);
			if( m == PEER_SETS_NAME ){
				strcpy(p->name, ((reg_set_name*)buf)->name);
				printf("Si e' connesso %s\n",p->name);
				//controllare se esiste un altro con lo stesso nome
				message_type m = NAME_ACCEPTED;
				send_data(sockt, (char*)&m, sizeof(m));
				p->state = NAME_SET;
				res = 0;
			}
			break;
		case NAME_SET:
			//ricevo la porta su cui ascolta
			sscanf(buf, "MY PORT IS %hu", &p->udp_port); 
		   	p->state = PEER_FREE;
		   	res = 1;
		   	printf("PEER %s riceve su porta UDP %hu \n",p->name,ntohs(p->udp_port));
			break;
		case PEER_FREE:
			//printf("PEER_FREE\n");
			sscanf(buf," %s %s",cmd,arg);
			//printf("cmd:%s arg:%s",cmd,arg);
			if( strcmp(buf,"LIST OF PEERS") == 0 ) {
				//printf("Richiesta lista di peer\n");
				res = send_list_of_peer(p->conn.socket); 
				//printf("la send_list_of_peer ha mandato %d\n",n);		
			} else if ( strcmp(cmd,"CONNECT") == 0) {
				printf("%s richiede connessione a %s\n",p->name,arg);
				connect_request(index,arg);
				res = 4;
			} else {
				res = -1;
			}
			break;
		case PEER_PLAYING:
			printf("PEER PLAYING\n");
			res = 5;
			break;
		default:
			res = -1;
	}
	return res;
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

int send_list_of_peer( int sockt )
{
	char* list = NULL;
	int n = get_peers_name(&list, listener->peers_connected);
	
	if( n < 0)
		return -1; 
	
	//printf("dim=%d --%s\n",n,list);
	
	if( n > 0)
		n = send_data(sockt, list, n);

	return n;
}

int connect_request(int sender_id, char* opponent_name )
{
	des_peer* p_sender = peers[sender_id];
	int index = get_index_peer_name(opponent_name);
	char buf[DIM_BUF];
	req_conn_peer r;


	if( index == -1 )
	{
		strcpy(buf,"CLIENT DOES NOT EXISTS");
		send_data(p_sender->conn.socket,buf,strlen(buf)+1);
		return -1;
	}

	r.t = htonl(REQ_CONN_FROM_PEER);
	r.peer_id = sender_id;
	strcpy(r.peer_name, p_sender->name);

//	sprintf(buf,"REQ_CONNECTION_FROM %s",sender->name);
//	send_data(peers[index]->conn.socket,buf,strlen(buf)+1);
	send_data(peers[index]->conn.socket,(char*)&r,sizeof(r));

	return 0;
}