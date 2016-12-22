#include "game.h"
#include "net_wrapper.h"
#include "messages.h"
#include <signal.h>


/***Variabili globali***/
int sock_serv_TCP;
int my_id = -1;
uint16_t my_udp_port;
char my_name[64];
const unsigned short DIM_BUFF = 512;
struct sockaddr_in my_addr;
battle_game my_grind;

int socket_udp;
char opposite_name[64];
struct sockaddr_in opposite_addr;
battle_game opposite_grind;
unsigned char opposite_ready = 0;

char mode = '>';
unsigned char arranging_ships = 0;
unsigned char your_turn = 1;
/*************************************/

void help();
void register_to_serv( int sd );
void execute_cmd(char* str, int sockt);
void remote_req(int sockt, char* buf);
void arrange_my_ship();
void disconnect();
void switch_mode();
void i_am_free(int sock);

int main( int argc, char* argv[] )
{
	int sd, ret;
	struct sockaddr_in serv_addr;
	char buf[DIM_BUFF];
	char* buf_rec = NULL;

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
	signal(SIGINT,disconnect);

	socket_udp = socket(AF_INET,SOCK_DGRAM,0);
	if( socket_udp == -1 ) {
		printf("Errore socket udp non aperta.\n");
		return -1;
	}

	register_to_serv(sd);
	help();

	init_game( &my_grind, 1, socket_udp );
	init_game( &opposite_grind, 0, socket_udp );

	FD_SET(STDIN_FILENO,&master);	//stdin
	FD_SET(sd,&master);
	FD_SET(socket_udp,&master);

	int max = (socket_udp>sd) ? socket_udp : sd;
	while(1) {
		printf("%c",mode);	
		fflush(stdout);	
		read_fds = master;
		select( max+1, &read_fds, NULL, NULL, NULL);

		if( FD_ISSET(sd, &read_fds) )
		{
			printf("\b");
			ret = recv_data(sd,&buf_rec);
			if( ret == -1 )	{
				printf("Il server ha chiuso la connessione\n");
				close(sd);
				exit(1);
			}
			remote_req(sd, buf_rec);
		} else if( FD_ISSET(socket_udp, &read_fds) ) {
			//printf("mess udp\n");
			recv_data(socket_udp,&buf_rec);
			remote_req(socket_udp, buf_rec);
		} else {
			/*Cattura tutta la linea fino al carattere a capo*/
			scanf(" %[^\n]s",buf);
			execute_cmd(buf,sd);
		}
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
		printf("   !show --> mostra le griglie\n");
		printf("   !quit --> disconnette il client dal server\n\n");
	}
}

void register_to_serv( int sd )
{
	char buf[DIM_BUFF];
	char* buf_rec = NULL;
	
	reg_set_name r_name = INIT_REG_SET_NAME;
	reg_set_udp_port r_udp = INIT_REG_SET_UDP_PORT;

	int ret = recv_data(sd, &buf_rec);
	if( ret ) {
		sscanf(buf_rec, "WELCOME YOUR ID IS %d", &my_id);
		printf("MY ID IS: %d \n",my_id);
	}
	else
		disconnect();
	
	for(;;) 
	{
		printf(">Inserisci il tuo nome: ");
		scanf("%s", buf);
		if( strlen(buf) > 63 )
			printf("Il tuo nome e' troppo lungo e verra' troncato");
		buf[63] = '\0';

		//r_name.t = htonl(PEER_SETS_NAME);
		strcpy(r_name.name,buf);

		r_name.peer_id = my_id;
		ret = send_data(sd, (char*)&r_name, sizeof(r_name));
		ret = recv_data(sd, &buf_rec); 

		/*si puo' togliere da qui forse*/
		message_type m;
		memcpy(&m, buf_rec, sizeof(m));

		if( m == NAME_ACCEPTED ){
			printf("Nome accettato\n");
			break;
		} else if ( m == NAME_REFUSED ) {
			printf("Il nome e' gia' usato da un altro peer. \n");
		} else {
			printf("Messaggio non riconosciuto.\n");
			disconnect();
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
	send_data( sd, (char*)&r_udp, sizeof(r_udp) );

	free(buf_rec);
}

void execute_cmd( char* str, int sockt )
{
	char cmd[10];
	char higher_str[65];
	char* buf_rec = NULL;
	req_conn_peer re;
	char col;
	char row;
	int ret;

	//printf("execute_cmd...\n");

	if( strlen(str) > 74 )
		return;

	sscanf(str," %s %[^\n]s",cmd, higher_str);
	//printf("%s %s\n",cmd,higher_str);

	if( strcmp(cmd,"!help") == 0 ) {
		help();
	} else if ( strcmp(str, "!quit") == 0 ){
		disconnect();
	} else if ( arranging_ships ) {
		arrange_my_ship(str);
	} else if ( strcmp(cmd, "!who") == 0 ) {
		simple_mess sm;
		sm.t = LIST_OF_PEERS;
		sm.peer_id = my_id;
		send_data(sockt,(char*)&sm,sizeof(sm));
		recv_data(sockt,&buf_rec);
		printf("Peer connessi al server:\n%s",buf_rec);
	} else if( strcmp(cmd, "!connect") == 0 ) {
		#ifdef DEBUG
		printf("mi connetto a %s\n",higher_str);
		#endif
		if( strcmp(higher_str,my_name) == 0 ){
			printf("Non puoi giocare con te stesso!\n");
			return;
		}

		re.t = REQ_CONN_TO_PEER;
		re.peer_id = my_id;
		strcpy(re.peer_name,higher_str);
		send_data(sockt,(char*)&re,sizeof(re));
	} else if( strcmp(cmd,"!shot")==0 ) {
		coordinate co;
		if( !opposite_ready ){
			printf("L'avversario non e' ancora pronto.\n");
			return;
		}

		if( !your_turn ){
			printf("Non è il tuo turno.\n");
			return;
		}

		sscanf(higher_str," %c %c",&col,&row);
		co.x = col;
		co.y = row;

		ret = shot_ship(&co, &opposite_grind);
		if( ret == -1 )
			printf("coordinate errate.\n");

		if( ret == -2 )
			printf("colpo gia' sparato.\n");

		if( ret >= 0 )
			your_turn = 0;
	}

	if( buf_rec )
		free(buf_rec);
	//printf("%c",mode);
	//fflush(stdout);
}

void remote_req( int sockt, char* buf )
{
	message_type m_type = GENERIC_ERR;
	response_conn_to_peer re;
	int ret;
	char c;
	memcpy((void*)&m_type, (void*)buf, sizeof(m_type));
//	m_type = m_type; //mettere una convert di libreria

	if( m_type == REQ_CONN_FROM_PEER ){
		/*il server invia erronamente un messaggio
		 *di connessione accettata mentre si e' in
		 *partita.*/
		if( mode == '#') return;
		
		printf(">Accetti connessione con %s? [S/N]: ", ((req_conn_peer*)buf)->peer_name);
		scanf(" %[^\n]c",&c);
		//scanf("%c",&c);
		if( c=='S' || c=='s' ) {
			//printf("Accettato\n");
			/*Copio i dati dell'avversario presenti nel messaggio
		 	*nelle variabili globali*/
			strcpy(opposite_name,((req_conn_peer*)buf)->peer_name);
			memcpy((void*)&opposite_addr, (void*)&(((req_conn_peer*)buf)->peer_addr), sizeof(opposite_addr));

			re.t = ACCEPT_CONN_FROM_PEER;		
		} else {
			//printf("Rifiutato\n");
			re.t = REFUSE_CONN_FROM_PEER;
		}
		re.opponent_id = ((req_conn_peer*)buf)->peer_id;
		re.peer_id = my_id;
		//convert_to_network_order(&re);
		send_data(sockt,(char*)&re,sizeof(re));	

		if( c=='S' || c=='s'){
			switch_mode();	
			connect(socket_udp, (struct sockaddr*)&opposite_addr, sizeof(opposite_addr));
			/*Il sistema inizia a sistemare la navi*/
			arranging_ships = 1; 
			printf("-------------------------------\n");
			printf("--Posiziona le tue navi\n");
		}	
	} else if ( m_type == CONN_TO_PEER_ACCEPTED ) { 
		/*il server invia erronamente un messaggio
		 *di connessione accettata mentre si e' in
		 *partita.*/
		if( mode == '#') return;

		/*Copio i dati dell'avversario presenti nel messaggio
		 *nelle variabili globali*/
		strcpy(opposite_name,((req_conn_peer*)buf)->peer_name);
		memcpy((void*)&opposite_addr, (void*)&(((req_conn_peer*)buf)->peer_addr), sizeof(opposite_addr));
		
		#ifdef DEBUG
		printf("avversario ip:%x udp_port:%d\n",htonl(opposite_addr.sin_addr.s_addr),ntohs(opposite_addr.sin_port));
		#endif
		//aggancio il socket udp all'avversario
		ret = connect(socket_udp, (struct sockaddr*)&opposite_addr, sizeof(opposite_addr));

		printf("Richiesta di connessione accettata %d\n",ret);
		switch_mode();

		/*Il sistema inizia a sistemare la navi*/
		arranging_ships = 1; 
		printf("-------------------------------\n");
		printf("--Posiziona le tue navi\n");
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
	} else if ( m_type == SHOT_SHIP ) {
		coordinate co;
		co.x = ((shot_mess*)buf)->col;
		co.y = ((shot_mess*)buf)->row;
		printf("\bavversario ha sparato: %c %c\n",co.x,co.y);
		ret = shot_ship(&co,&my_grind);
		/*gestire errori in modo più pulito anche 
		  aggiungendo nuovi messaggi*/
		show_grind(&my_grind, &opposite_grind);

		if( ret == 2 ){
			printf("Hai perso! :( \n");
			i_am_free(sock_serv_TCP);			
		}

		your_turn = 1;
	} else if ( m_type == SHIP_ARRANGED ) {
		printf("\b%s ha posizionato tutte le navi\n",opposite_name);
		opposite_ready = 1;
	} else if ( m_type == SHIP_HIT ) {
		coordinate co;
		co.x = ((shot_mess*)buf)->col;
		co.y = ((shot_mess*)buf)->row;
		set_hit(&co,&opposite_grind);
		printf("\bcolpito!\n");
		show_grind(&my_grind,&opposite_grind);
	} else if ( m_type == SHIP_MISS ) {
		coordinate co;
		co.x = ((shot_mess*)buf)->col;
		co.y = ((shot_mess*)buf)->row;
		set_miss(&co,&opposite_grind);
		printf("\bbuco nell'acqua.\n");
		show_grind(&my_grind,&opposite_grind);
	} else if ( m_type == YOU_WON ){
		printf("\bHai vinto la partita! :D\n");
		i_am_free(sock_serv_TCP);
	} else if( m_type == OPPONENT_DISCONNECTED ){
		printf("\bIl tuo avversario si è disconnesso.\n");
		mode = '>'; arranging_ships=0;		
	} else {
		printf("Messaggio non riconosciuto\n");
	}
}

void switch_mode(){
	if( mode == '>' ) mode = '#';
	else mode = '>';
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
		arranging_ships = 0;
		if( opposite_ready )
			your_turn = 1;
		else
			your_turn = 0;
		printf("Navi posizionate con successo.\n");
	}
	/*#ifdef DEBUG
	printf("%c %c\n",col,row);
	printf("debug=%d\n",x);
	#endif */
	show_grind(&my_grind,NULL);
//	}
//1	printf("--NAVI POSIZIONATE\n");
}

void i_am_free( int sock )
{
	simple_mess sm = { I_AM_FREE, my_id };
	opposite_ready = 0;
	opposite_name[0] = '\0';
	send_data(sock, (char*)&sm, sizeof(sm));
	switch_mode();
}

void disconnect( int sig )
{
	#ifdef DEBUG
	printf("disconnessione\n");
	#endif
	simple_mess m;
	m.t = DISCONNECT;
	m.peer_id = my_id;
	send_data(sock_serv_TCP, (char*)&m, sizeof(m));
	close(sock_serv_TCP);
	printf("\n\n");
	exit(0);
}