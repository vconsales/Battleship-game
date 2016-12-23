#include "net_wrapper.h"
#include "peer_manager.h"
#include "messages.h"

/***Variabili globali***/
int sock_serv;
my_buffer my_buf = INIT_MY_BUFFER;
/**********************/

void requests_manager();
int analyze_message( int sockt, char* buf );
int send_welcome( int sockt, int id );
int send_list_of_peer( int sockt );
int connect_request( int sender_id, char* opponent_name );
void peer_quit( int id );

int main( int argc, char* argv[] )
{
	uint16_t port = 0;

	if( argc < 2 ) {
		printf("Numero di porta non inserito!\n");
		exit(1);
	}

	if( !init_peer_manager() ){
		printf("Errore interno: peer_manager\n");
		exit(1);
	}

	/*Ottengo porta*/
	sscanf( argv[1], "%hu", &port );

	sock_serv = open_serverTCP(port); 
	if( sock_serv == -1 ){
		exit(1);
	}

	/*aggiungo handler al segnale generato da ctrl+c*/
	//signal(SIGINT,close_serverTCP);

	requests_manager();

	close(sock_serv);
	return 0;
}

void requests_manager()
{
	int fdmax, i, id, ret, sd;
	des_peer* pe;

	fd_set master; /*set principale*/
	fd_set read_fds; /*set di lettura*/
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	/*aggiungo listener nel set*/
	FD_SET( sock_serv, &master ); 
	fdmax = sock_serv;

	while( 1 )
	{
		read_fds = master;
		select( fdmax+1, &read_fds, NULL, NULL, NULL);

		/*scorro i descrittori*/
		for( i=0; i<=fdmax; i++ )
		{
			if( FD_ISSET(i, &read_fds) )
			{
				if( i == sock_serv ){
					//ret = listen(listener->socket, 10);	 //forse non serve				
					id = add_peer();
					if( id != -1 ){
						pe = get_peer(id);
						sd = accept_serverTCP( sock_serv, &pe->conn );
						send_welcome(sd, id);

						FD_SET( sd, &master );
						if( sd > fdmax )
							fdmax = sd;
					} else
						printf("Connessione rifiutata. Troppi peer connessi\n");		
				} else {
					ret = recv_data(i,&my_buf);
					if( ret == -1 ){						
						id = get_index_peer_sock(i);
						peer_quit(id);

						FD_CLR(i,&master);
						close(i);
						if( i == fdmax)
							fdmax--;
					} else {
						/*Il messaggio ricevuto viene analizzato secondo
						 *il protocollo scelto. Se necessario vengono
						 *aggiornate le strutture dati associate ai peer*/
						//printf("Ricevuto messaggio\n");
						ret = analyze_message(i, my_buf.buf);
						if( ret == -1 ){
							FD_CLR(i,&master);
							close(i);
							if( i == fdmax)
								fdmax--;
						}
					}
				}
			}
		}
	}	
}

/********************************************************
* Analizza un messaggio mandato da un peer al server
* Restituisce:
*   -  0 in caso di successo
*   - -1 in caso di errore di protezione
*   - -2 in caso di messaggio non riconosciuto
*/
int analyze_message( int sockt, char* buf )
{
	des_peer* p;

	message_type m;
	req_conn_peer re;

	//printf("analizzo messaggio\n");
	//int index = get_index_peer_sock(sockt);
	int index = ((simple_mess*)buf)->peer_id;

	if( !is_valid_id(index) ) {
		printf("Peer con id=%d non trovato\n",index);
		return -1;
	}
	p = get_peer(index);

	if( p->conn.socket != sockt ) {
		printf("Errore di sicurezza. Messaggio rifiutato\n");
		return -1;
	}

	memcpy(&m, buf, sizeof(m));

	if( m == DISCONNECT_GAME ) {
		set_peer_free(index);
	}else if( m == PEER_SETS_NAME ) {
		if( p->state != UNSET )
			return -1;

		if( get_index_peer_name(((reg_set_name*)buf)->name) != -1 )
		{
			m = NAME_REFUSED;
			send_data(sockt, (char*)&m, sizeof(m));
		} else {
			m = NAME_ACCEPTED;
			strcpy(p->name, ((reg_set_name*)buf)->name);
			send_data(sockt, (char*)&m, sizeof(m));
			p->state = NAME_SET;
		}
	} else if( m == PEER_SETS_UDP_PORT) {
		if( p->state != NAME_SET )
			return -1;

	   	p->state = PEER_FREE;
	   	p->udp_port = ((reg_set_udp_port*)buf)->udp_port;
	   	printf("PEER %s riceve su porta UDP %hu \n",p->name,ntohs(p->udp_port));
	} else if ( m == LIST_OF_PEERS ) {
		#ifdef DEBUG
		printf("Richiesta lista di peer\n");
		#endif
		send_list_of_peer(p->conn.socket); 
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
		p = get_peer(opponent_id); 
		send_data(p->conn.socket,(char*)&re,sizeof(re));

		p->state = PEER_PLAYING;
		p->opponent_id = index;

		p = get_peer(index);
		p->state = PEER_PLAYING;
		p->opponent_id = opponent_id;
	} else if ( m == REFUSE_CONN_FROM_PEER ) {
		re.t = CONN_TO_PEER_REFUSED;
		p = get_peer( ((response_conn_to_peer*)buf)->opponent_id );
		send_data(p->conn.socket,(char*)&re,sizeof(re));
	} else {
		return -2;
	}

	return 0;
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
	int n = get_peers_name(&list);
	
	if( n < 0)
		return -1; 
	
	//printf("dim=%d --%s\n",n,list);
	
	if( n > 0)
		n = send_data(sockt, list, n);

	return n;
}

int connect_request(int sender_id, char* opponent_name )
{
	des_peer* p_sender = get_peer(sender_id);
	int index = get_index_peer_name(opponent_name); 
	req_conn_peer r;

	if( index == -1 ) {
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

	send_data(get_peer(index)->conn.socket,(char*)&r,sizeof(r));

	return 0;
}

void peer_quit( int id )
{
	des_peer* p = get_peer(id);
	if( !p ) 
		return;

	printf("Il peer %s si è disconnesso dal server\n",p->name);
	set_peer_free(id);
	remove_peer(id);
}

void set_peer_free( int id )
{
	des_peer* p = get_peer(id);
	des_peer* opponent;
	message_type m;

	if( !p ) return;

	if( p->state == PEER_PLAYING )
	{
		m = OPPONENT_DISCONNECTED;
		opponent = get_peer(p->opponent_id);
		send_data(opponent->conn.socket,(char*)&m,sizeof(m));
		opponent->state = PEER_FREE;
		p->state = PEER_FREE;
		printf("%s si è disconnesso dalla partita\n",p->name);
		printf("%s si è disconnesso dalla partita\n",opponent->name);
	}
}