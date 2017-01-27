#include "game.h"
#include "net_wrapper.h"
#include "messages.h"
#include "battle_client.h"
#include <signal.h>

void quit(int sig);
void help();
void register_to_serv( int sd );
void execute_cmd( char* str );
void remote_req( int sockt, char* buf );
void arrange_my_ship();
void disconnect(int show_msg);
void switch_mode();
void print_list_of_peers( char* buf );
void shot_cmd( char* str );
void connect_cmd( char* str );
void who_cmd();

void stampa_indirizzo( struct sockaddr_in *addr )
{
	char str[16];
	inet_ntop(AF_INET, &addr->sin_addr, str, 16);
	printf("ip: %s",str);
	printf("porta: %hu", ntohs(addr->sin_port));
}

unsigned char playing_game()
{
	unsigned char mask = 0x80;
	return state & mask;
}

void set_playing_game( int s )
{
	unsigned char mask = 0x80;
	/*se devo settare metto ad 1 il
         *bit n.7 altrimenti lo azzero*/	
	if ( s )
		state |= mask;
	else
		state &= ~mask;
}

unsigned char is_opposite_ready()
{
	unsigned char mask = 0x40;
	return state & mask;
}

void set_opposite_ready( int s )
{
	unsigned char mask = 0x40;
	if ( s )
		state |= mask;
	else
		state &= ~mask;
}

unsigned char arranging_ships()
{
	unsigned char mask = 0x20;
	return state & mask;
}

void set_arranging_ships( int s )
{
	unsigned char mask = 0x20;
	if ( s )
		state |= mask;
	else
		state &= ~mask;
}

unsigned char is_my_turn()
{
	unsigned char mask = 0x10;
	return state & mask;
}

void set_my_turn( int s )
{
	unsigned char mask = 0x10;
	if ( s )
		state |= mask;
	else
		state &= ~mask;
}

unsigned char udp_port_registred()
{
	unsigned char mask = 0x04;
	return state & mask;
}

void set_udp_port_registred( int s )
{
	unsigned char mask = 0x04;
	if ( s )
		state |= mask;
	else
		state &= ~mask;
}

unsigned char name_registred()
{
	unsigned char mask = 0x02;
	return state &= mask;
}

void set_name_registred( int s )
{
	unsigned char mask = 0x02;
	if ( s )
		state |= mask;
	else
		state &= ~mask;
}

unsigned char id_received()
{
	unsigned char mask = 0x01;
	return state & mask;
}

void set_id_received( int s )
{
	unsigned char mask = 0x01;
	if ( s )
		state |= mask;
	else
		state &= ~mask;
}

int main( int argc, char* argv[] )
{
	int sd, ret;
	struct sockaddr_in serv_addr;

	char buf[DIM_BUFF];
	struct timeval timeout;

	fd_set master; /*set principale*/
	fd_set read_fds; /*set di lettura*/
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	if( argc < 3 )
	{
		printf("uso: ./battle_client <host remoto> <porta>\n");
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	ret = inet_pton(AF_INET, argv[1], (void*)&serv_addr.sin_addr);
	if( ret!=1 )
	{
		printf("Errore ip del server non valido.\n");
		return -1;
	}
	/*ricavo la porta del server*/
	sscanf(argv[2],"%hu", &serv_addr.sin_port);
	/*converto in big endian*/
	serv_addr.sin_port = htons(serv_addr.sin_port); 

      	/*apro socket tcp per la connessione con il server*/
	sd = socket(AF_INET, SOCK_STREAM, 0);
	sock_serv_TCP = sd;

	if( sd == -1 )
	{
		perror("Errore socket non aperta");
		return -1;
	}

	ret = connect(sd, (struct sockaddr*)&serv_addr, sizeof(serv_addr) );
	if( ret == -1 )
	{
		perror("Errore connect");
		close(sd);		
		exit(1);
	} else 
		printf("Connesso correttamente al server %s\n",argv[1]);

	/*assegno handler al segnale generato da CTRL+C*/
	signal(SIGINT,quit);

      	/*apro socket udp per la comunicazione p2p*/
	socket_udp = socket(AF_INET,SOCK_DGRAM,0);
	if( socket_udp == -1 ) {
		printf("Errore socket udp non aperta.\n");
		return -1;
	}

      	/*Registrazione al server*/
	register_to_serv(sd);

      	/*Mostro i comandi disponibili*/
	help();

	/*Inizializzo il gioco*/
	init_game( &my_grind, 1, socket_udp );
	init_game( &opposite_grind, 0, socket_udp );

      
	FD_SET(STDIN_FILENO,&master);	
	FD_SET(sd,&master);
	FD_SET(socket_udp,&master);

	int max = (socket_udp>sd) ? socket_udp : sd;
	while(1) {
		printf("%c",mode);	
		fflush(stdout);	
		fflush(stdin);
		read_fds = master;

            /*se siamo nella modalita game uso il timeout*/
		if( mode == '#')
			select( max+1, &read_fds, NULL, NULL, &timeout);
		else
			select( max+1, &read_fds, NULL, NULL, NULL);

		if( FD_ISSET(sd, &read_fds) )
		{
                  /*il messaggio è stato mandato dal server*/
            
                  printf("\b"); /*cancella ">" */
			ret = recv_data(sd, &my_buf);
			if( ret == -1 )	{
				printf("Il server ha chiuso la connessione\n");
				close(sd);
				exit(1);
			}
			convert_to_host_order(my_buf.buf);
			remote_req(sd, my_buf.buf);
		} else if( FD_ISSET(socket_udp, &read_fds) ) {
			recv_data(socket_udp,&my_buf);
			convert_to_host_order(my_buf.buf);
			remote_req(socket_udp, my_buf.buf);
		} else if( FD_ISSET(STDIN_FILENO,&read_fds) ) {
			/*Cattura tutta la linea fino al carattere a capo*/
			scanf(" %[^\n]s",buf);
			/*esegue il comando scritto da tastiera*/
			execute_cmd(buf);
		} else {
			printf("\bTempo scaduto.\n");
			disconnect(1);
		}
            timeout.tv_sec = N_SECONDS;
	}

	close(sd);
	return 0;
}

void help()
{
	if( mode == '>' ){
		printf("\n");
		printf("Sono disponibili i seguenti comandi:\n");
		printf("   !help --> mostra l'elenco dei comandi disponibili\n");
		printf("   !who --> mostra l'elenco dei client connessi al server\n");
		printf("   !connect username --> avvia una partita con l'utente username\n");
		printf("   !quit --> disconnette il client dal server\n\n");
	} else {
		printf("Sono disponibili i seguenti comandi:\n");
		printf("   !help --> mostra l'elenco dei comandi disponibili\n");
		printf("   !shot riga colonna --> spara alle coordinate indicate\n");
		printf("   !show --> mostra le griglie di gioco\n");
		printf("   !disconnect --> disconnette il client dall'attuale partita\n\n");
	}
}

void register_to_serv( int sd )
{
	char buf[DIM_BUFF];
	
	reg_set_name r_name = INIT_REG_SET_NAME;
	reg_set_udp_port r_udp = INIT_REG_SET_UDP_PORT;

	int ret = recv_data(sd, &my_buf);
	//printf("byte ricevuti %d\n",ret);
	if( ret ) {
		convert_to_host_order(my_buf.buf);

		if( ((simple_mess*)my_buf.buf)->t != WELCOME_MESS ){
			printf("Messaggio di welcome non ricevuto\n");
			quit(1);
		}

		my_id = ((simple_mess*)my_buf.buf)->peer_id;
		#ifdef DEBUG		
		printf("MY ID IS: %d \n",my_id);
		#endif
	}
	else {
            printf("Errore nella registrazione\n");		
            disconnect(1);
	}
	r_name.peer_id = my_id;
	convert_to_network_order(&r_name);

	for(;;) 
	{
		printf(">Inserisci il tuo nome: ");
		scanf("%s", buf);
		if( strlen(buf) > 63 ){
			printf("Il tuo nome e' troppo lungo e verra' troncato");
			buf[63] = '\0';
		}
		strcpy(my_name,buf);
		strcpy(r_name.name,buf);
		ret = send_data(sd, (char*)&r_name, sizeof(r_name));
		if( ret == -1 )
			quit(1);

		ret = recv_data(sd, &my_buf); 
		if( ret == -1 ){
			printf("Registrazione fallita.\n");
			quit(1);		
		}
		convert_to_host_order(my_buf.buf);

		message_type m;
		memcpy(&m, my_buf.buf, sizeof(m));

		if( m == NAME_ACCEPTED ){
			printf("Nome accettato\n");
			break;
		} else if ( m == NAME_REFUSED ) {
			printf("Il nome e' gia' usato da un altro peer. \n");
			buf[0] = '\0';
		} else {
			printf("Messaggio non riconosciuto.\n");
			quit(1);
		}
	}

_set_udp_port:
	printf(">Inserisci la tua porta UDP: ");
	scanf("%hu",&my_udp_port);
	my_udp_port = htons(my_udp_port);

	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = my_udp_port;

	ret = bind(socket_udp,(struct sockaddr*)&my_addr,sizeof(my_addr));
	if( ret == -1 )
	{
		printf("La porta udp scelta non e' disponibile.\n");
		goto _set_udp_port;
	}

	r_udp.peer_id = my_id;
	r_udp.udp_port = my_udp_port;
	//sprintf(buf, "MY PORT IS %hu",my_udp_port);
	convert_to_network_order(&r_udp);
	send_data( sd, (char*)&r_udp, sizeof(r_udp) );
}

void execute_cmd( char* str )
{
	char cmd[10];
	char higher_str[65];

	if( strlen(str) > 74 )
		return;

/*	fflush(stdin);
	printf("La stringa ricev: %s\n",str );*/
	//sscanf(str," %s %s",cmd,higher_str);
	sscanf(str," %s %[^\n]s",cmd, higher_str);
	//printf("%s %s\n",cmd,higher_str);

	if( strcmp(cmd,"!help") == 0 ) {
		help();
	} else if ( strcmp(str, "!quit") == 0 ){
		quit(0);
	} else if ( strcmp(str, "!disconnect") == 0 ) {
		disconnect(1);
	} else if ( arranging_ships() ) {
		arrange_my_ship(str);
	} else if ( strcmp(cmd, "!who") == 0 ) {
		who_cmd();
	} else if( strcmp(cmd, "!connect") == 0 ) {
		connect_cmd(higher_str);
	} else if( strcmp(cmd,"!shot")==0 ) {
		if( playing_game() )
			shot_cmd(higher_str);
		else
			printf("Gioco non iniziato\n");
	} else 
		printf("\bComando non riconosciuto.\n");
}

void remote_req( int sockt, char* buf )
{
	message_type m_type;
	response_conn_to_peer re;
	int ret;
	char c;
	memcpy((void*)&m_type, (void*)buf, sizeof(m_type));
	
	if( m_type == REQ_CONN_FROM_PEER ){
		/*se il server invia erronamente un messaggio
		 *di connessione accettata mentre si e' in partita.*/
		if( mode == '#') return;
		
		printf(">Accetti connessione con %s? [S/N]: ", ((req_conn_peer*)buf)->peer_name);
		scanf(" %[^\n]c",&c);
		//scanf("%c",&c);
		if( c=='S' || c=='s' ) {
			/*Copio i dati dell'avversario presenti nel messaggio
		 	*nelle variabili globali*/
			strcpy(opposite_name,((req_conn_peer*)buf)->peer_name);
			memcpy((void*)&opposite_addr, (void*)&(((req_conn_peer*)buf)->peer_addr), sizeof(opposite_addr));
			#ifdef DEBUG
			stampa_indirizzo(&opposite_addr);
			#endif
			re.t = ACCEPT_CONN_FROM_PEER;		
		} else {
			//printf("Rifiutato\n");
			re.t = REFUSE_CONN_FROM_PEER;
		}
		re.opponent_id = ((req_conn_peer*)buf)->peer_id;
		re.peer_id = my_id;
		convert_to_network_order(&re);
		send_data(sockt,(char*)&re,sizeof(re));	

		if( c=='S' || c=='s'){
			switch_mode();	
			connect(socket_udp, (struct sockaddr*)&opposite_addr, sizeof(opposite_addr));
			/*Il sistema inizia a sistemare la navi*/
			set_arranging_ships(1); 
			printf("------------------------------------\n");
			printf("-Posiziona le tue navi\n");
		}	
	} else if ( m_type == CONN_TO_PEER_ACCEPTED ) { 
		/*il server invia erroneamente un messaggio
		 *di connessione accettata mentre si e' in
		 *partita.*/
		if( mode == '#') return;

		/*Copio i dati dell'avversario presenti nel messaggio
		 *nelle variabili globali*/
		strcpy(opposite_name,((req_conn_peer*)buf)->peer_name);
		memcpy((void*)&opposite_addr, (void*)&(((req_conn_peer*)buf)->peer_addr), sizeof(opposite_addr));	
		#ifdef DEBUG
		stampa_indirizzo(&opposite_addr);
		#endif
		/*aggancio socket udp all'avversario*/
		ret = connect(socket_udp, (struct sockaddr*)&opposite_addr, sizeof(opposite_addr));
            
            if( ret == 0 )
		      printf("Richiesta di connessione accettata\n");
            else {
                  perror("Errore di connessione p2p");
                  disconnect(1);   
            }
		switch_mode();

		/*Il sistema inizia a sistemare la navi*/
		set_arranging_ships(1); 
		printf("------------------------------------\n");
		printf("-Posiziona le tue navi\n");
	} else if ( m_type == CONN_TO_PEER_REFUSED ) { 
		/*il server invia erronamente un messaggio
		 *di connessione accettata mentre si e' in
		 *partita.*/
		if( mode == '#') return;
		printf("Richiesta di connessione rifiutata\n");
	} else if ( m_type == PEER_IS_NOT_FREE ) {
		printf("Il peer indicato e' gia' occupato in un'altra partita.\n");
	} else if ( m_type == PEER_DOES_NOT_EXIST ) {
		printf("Il peer indicato non esiste\n");
	} else if ( m_type == RES_LIST_OF_PEERS ) {
		print_list_of_peers(my_buf.buf);
	} else if ( m_type == SHOT_SHIP ) {
		coordinate co;
		co.x = ((shot_mess*)buf)->col;
		co.y = ((shot_mess*)buf)->row;
		printf("\bavversario ha sparato: %c %c\n",co.x,co.y);
		ret = shot_ship(&co,&my_grind);
		/*gestire errori in modo più pulito anche 
		  aggiungendo nuovi messaggi*/
		show_grinds(&my_grind, &opposite_grind);

		if( ret == 2 ){
			printf("Hai perso! :( \n");
			disconnect(0);			
		}

		set_my_turn(1);
	} else if ( m_type == SHIP_ARRANGED ) {
		printf("\b%s ha posizionato tutte le navi\n",opposite_name);
		set_opposite_ready(1);
	} else if ( m_type == SHIP_HIT ) {
		coordinate co;
		co.x = ((shot_mess*)buf)->col;
		co.y = ((shot_mess*)buf)->row;
		set_hit(&co,&opposite_grind);
		printf("\bcolpito!\n");
		show_grinds(&my_grind,&opposite_grind);
	} else if ( m_type == SHIP_MISS ) {
		coordinate co;
		co.x = ((shot_mess*)buf)->col;
		co.y = ((shot_mess*)buf)->row;
		set_miss(&co,&opposite_grind);
		printf("\bbuco nell'acqua.\n");
		show_grinds(&my_grind,&opposite_grind);
	} else if ( m_type == YOU_WON ){
		printf("\bHai vinto la partita! :D\n");
		disconnect(0);
		/*mode = '>';
		state &= 0x0F;*/
	} else if( m_type == OPPONENT_DISCONNECTED ){
		if( get_n_ship_hit(&opposite_grind) != SHIP_NUMBER )
			printf("\bIl tuo avversario si è disconnesso. Hai vinto la partita :D!\n");
		mode = '>'; 
		state &= 0x0F;		
	} else if( m_type == SERVER_QUIT ) {
		printf("Il server è stato chiuso\n");
		quit(0);
	} else {
		printf("Messaggio non riconosciuto %s\n",buf);
	}
}

void switch_mode(){
	if( mode == '>' ) mode = '#';
	else mode = '>';
	init_game( &my_grind, 1, socket_udp );
	init_game( &opposite_grind, 0, socket_udp );
}

void print_list_of_peers( char* buf )
{
	res_list_peers *re = (res_list_peers*)buf;
	printf("\bPeer connessi al server:\n%s",re->list);
}

void who_cmd()
{
	simple_mess sm;
	sm.t = REQ_LIST_OF_PEERS;
	sm.peer_id = my_id;

	convert_to_network_order(&sm);
	send_data(sock_serv_TCP,(char*)&sm,sizeof(sm));
}

void connect_cmd( char* str )
{
	req_conn_peer re;
	if( strcmp(str,my_name) == 0 ){
		printf("Non puoi giocare con te stesso!\n");
		return;
	}
	re.t = REQ_CONN_TO_PEER;
	re.peer_id = my_id;
	strcpy(re.peer_name,str);

	convert_to_network_order(&re);
	send_data(sock_serv_TCP,(char*)&re,sizeof(re));
}

void shot_cmd( char* str )
{
	int ret;
	char col;
	char row;
	coordinate co;
	if( !is_opposite_ready() ){
		printf("L'avversario non e' ancora pronto.\n");
		return;
	}

	if( !is_my_turn() ){
		printf("Non è il tuo turno.\n");
		return;
	}

	sscanf(str," %c %c",&col,&row);
	co.x = col;
	co.y = row;

	ret = shot_ship(&co, &opposite_grind);
	if( ret == -1 )
		printf("coordinate errate.\n");

	if( ret == -2 )
		printf("colpo gia' sparato.\n");

	if( ret >= 0 )
		set_my_turn(0);
}

void arrange_my_ship(char* str)
{
	coordinate co;
	char col;
	char row;
	int res = 0;

	sscanf(str," %c %c",&col,&row);
	co.x = col;
	co.y = row;
	res = set_ship(&co,&my_grind);
	if( res < 0 )
		printf("Errore inserimento coordinate\n");
	else if ( res == 1 ) {
		set_arranging_ships(0);
		set_playing_game(1);
		if( is_opposite_ready() ){
			printf("Gioco iniziato. Spara!\n");
			set_my_turn(1);
		} else {
			set_my_turn(0);
			printf("Navi posizionate con successo. Aspetta l'avversario.\n");
            	}
	}
	show_grinds(&my_grind,NULL);
}

void disconnect( int show_msg )
{
	simple_mess m;
	m.t = DISCONNECT_GAME;
	m.peer_id = my_id;

	convert_to_network_order(&m);
	send_data(sock_serv_TCP, (char*)&m, sizeof(m));

	opposite_name[0] = '\0';
	/*azzero i bit riguardanti lo stato della partita*/
	state &= 0x0F; 

    	if( show_msg ){
		printf("Disconnessione avvenuta con successo.\n");
	}
	mode = '>';
}

void quit( int sig )
{
	disconnect(1);
	free(my_buf.buf);
	close(sock_serv_TCP);
	printf("\n\n");
	exit(0);
}