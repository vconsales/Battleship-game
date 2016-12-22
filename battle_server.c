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

	while( 1 )
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
						int ind_close = get_index_peer_sock(j);
						if( peers[ind_close]->state == PEER_PLAYING ){
							/**invio messaggio peer avversario si è disconnesso**/
							peers[ind_close]->state = PEER_FREE;
						}

						//remove_peer_having_sock(j);
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
						if( ret == -1 ){
							printf("chiudo il socket %d \n",j);
							close(j);
							FD_CLR(j,&master);
						}
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
	int res = -1;

	message_type m;
	req_conn_peer re;

	//printf("analizzo messaggio\n");
	//int index = get_index_peer_sock(sockt);
	int index = ((simple_mess*)buf)->peer_id;

	if( !is_valid_id(index) ) {
		printf("Peer con id=%d non trovato\n",index);
		return -1;
	}
	p = peers[index];

	if( p->conn.socket != sockt ) {
		printf("Errore di sicurezza. Messaggio rifiutato\n");
		return -2;
	}

	memcpy(&m, buf, sizeof(m));
	res = m;

	if( m == DISCONNECT ) {
		if( p->state == PEER_PLAYING )
		{
			m = OPPONENT_DISCONNECTED;
			index = p->opponent_id;
			p = peers[index];
			send_data(p->conn.socket,(char*)&m,sizeof(m));
		}
		printf("%s si è disconnesso\n",p->name);
		return -1;
	}else if( m == PEER_SETS_NAME ) {
		if( p->state != UNSET )
			return -1;

		strcpy(p->name, ((reg_set_name*)buf)->name);
		printf("Si e' connesso %s\n",p->name);
		//controllare se esiste un altro con lo stesso nome
		m = NAME_ACCEPTED;
		send_data(sockt, (char*)&m, sizeof(m));
		p->state = NAME_SET;
	} else if( m == PEER_SETS_UDP_PORT) {
		if( p->state != NAME_SET )
			return -1;

	   	p->state = PEER_FREE;
	   	p->udp_port = ((reg_set_udp_port*)buf)->udp_port;
	   	printf("PEER %s riceve su porta UDP %hu \n",p->name,ntohs(p->udp_port));
	} else if ( m == LIST_OF_PEERS ) {
		printf("Richiesta lista di peer\n");
		res = send_list_of_peer(p->conn.socket); 
	} else if( m == REQ_CONN_TO_PEER) {
		if( p->state != PEER_FREE )
			return -1;

		printf("%s richiede connessione a %s\n",p->name,((req_conn_peer*)buf)->peer_name);
		connect_request(index,((req_conn_peer*)buf)->peer_name);
	} else if ( m == ACCEPT_CONN_FROM_PEER ) {
		re.t = CONN_TO_PEER_ACCEPTED;		
		/* Ricopio i dati del peer che ha accettato nel messaggio di risposta*/
		re.peer_id = index;
		strcpy(re.peer_name,p->name);
		memcpy(&re.peer_addr,&(p->conn.cl_addr),sizeof(re.peer_addr));
		/*la porta udp è diversa della porta tcp della connessione client-server*/
		re.peer_addr.sin_port = p->udp_port;

		int opponent_id = ((response_conn_to_peer*)buf)->opponent_id;
		/*mando il messaggio all'avversario*/
		p = peers[opponent_id]; 
		send_data(p->conn.socket,(char*)&re,sizeof(re));

		p->state = PEER_PLAYING;
		p->opponent_id = index;

		p = peers[index];
		p->state = PEER_PLAYING;
		p->opponent_id = opponent_id;
	} else if ( m == REFUSE_CONN_FROM_PEER ) {
		re.t = CONN_TO_PEER_REFUSED;
		p = peers[((response_conn_to_peer*)buf)->opponent_id];
		send_data(p->conn.socket,(char*)&re,sizeof(re));
	} else {
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
	req_conn_peer r;


	if( index == -1 ) {
		//strcpy(buf,"CLIENT DOES NOT EXISTS");
		//send_data(p_sender->conn.socket,buf,strlen(buf)+1);
		r.t = PEER_DOES_NOT_EXIST;
		send_data(p_sender->conn.socket,(char*)&r,sizeof(r));
		return -1;
	}

	if( p_sender->state == PEER_PLAYING ) {
		r.t = PEER_IS_NOT_FREE;
		send_data(p_sender->conn.socket,(char*)&r,sizeof(r));
		return -2;
	}

	r.t = REQ_CONN_FROM_PEER;
	r.peer_id = sender_id;
	strcpy(r.peer_name, p_sender->name);
	memcpy((void*)&r.peer_addr,(void*)&(p_sender->conn.cl_addr),sizeof(r.peer_addr));
	r.peer_addr.sin_port = p_sender->udp_port;

//	sprintf(buf,"REQ_CONNECTION_FROM %s",sender->name);
//	send_data(peers[index]->conn.socket,buf,strlen(buf)+1);
	send_data(peers[index]->conn.socket,(char*)&r,sizeof(r));

	return 0;
}