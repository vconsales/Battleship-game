#include "game.h"
#include "TCP.h"
#include "messages.h"

#define DEBUG


/***Variabili globali***/
int my_id = -1;
uint16_t my_udp_port;
char my_name[64];
int sock_serv_TCP;
const unsigned short DIM_BUFF = 512;
char mode = '>';
battle_game my_grind;

char opposite_name[64];
uint16_t opposite_udp_port;
struct sockaddr_in opposite_addr;
battle_game opposite_grind;
/*************************************/

void help();
void register_to_serv( int sd );
void execute_cmd(char* str, int sockt);
void remote_req(int sockt, char* buf);
void disconnect();
void switch_mode();

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
		printf("Errore socket non aperta \n");
		return -1;
	}

	ret = connect(sd, (struct sockaddr*)&serv_addr, sizeof(serv_addr) );
	if( ret == -1 )
	{
		printf("Errore connect.\n");
		close(sd);		
		exit(1);
	} else 
		printf("Connesso correttamente al server %s\n",argv[1]);

	register_to_serv(sd);
	help();

	FD_SET(STDIN_FILENO,&master);	//stdin
	FD_SET(sd,&master);

	while(1) {
		printf("%c",mode);	
		fflush(stdout);	
		read_fds = master;
		select( sd+1, &read_fds, NULL, NULL, NULL);

		if( FD_ISSET(sd, &read_fds) )
		{
			printf("\b");
			recv_data(sd,&buf_rec);
			//printf("%s\n",buf_rec);
			//printf("\b");
			remote_req(sd, buf_rec);
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
	
	reg_set_name r = INIT_REG_SET_NAME;
	r.peer_id = my_id;

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

		//r.t = htonl(PEER_SETS_NAME);
		strcpy(r.name,buf);

		ret = send_data( sd, (char*)&r, sizeof(r) );
		ret = recv_data(sd, &buf_rec); 

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

	printf(">Inserisci la tua porta UDP: ");
	scanf("%hu",&my_udp_port);
	my_udp_port = htons(my_udp_port);

	sprintf(buf, "MY PORT IS %hu",my_udp_port);
	send_data( sd, buf, strlen(buf)+1 );

	free(buf_rec);
}

void execute_cmd( char* str, int sockt )
{
	char lower_str[10];
	char higher_str[65];
	char* buf = NULL;
	req_conn_peer re;

	printf("execute_cmd...\n");

	if( strlen(str) > 74 )
		return;

	sscanf(str," %s %s",lower_str, higher_str);
	//printf("%s %s\n",lower_str,higher_str);

	if( strcmp(str,"!help") == 0 ) {
		help();
	} else if ( strcmp(str, "!who") == 0 ) {
		strcpy(higher_str,"LIST OF PEERS");
		send_data(sockt,higher_str,strlen(higher_str)+1);
		recv_data(sockt,&buf);
		printf("Peer connessi al server:\n%s",buf);
	} else if ( strcmp(str, "!quit") == 0 ){
		disconnect();
	} else if( strcmp(lower_str, "!connect") == 0 ) {
		#ifdef DEBUG
		printf("mi connetto a %s\n",higher_str);
		#endif
		re.t = REQ_CONN_TO_PEER;
		re.peer_id = my_id;
		strcpy(re.peer_name,higher_str);
		send_data(sockt,(char*)&re,sizeof(re));
	}

	if( buf )
		free(buf);
	//printf("%c",mode);
	//fflush(stdout);
}

void remote_req( int sockt, char* buf )
{
	message_type m_type = GENERIC_ERR;
	response_conn_to_peer re;

	char c;
	memcpy((void*)&m_type, (void*)buf, sizeof(m_type));
	m_type = m_type; //mettere una convert di libreria

	if( m_type == REQ_CONN_FROM_PEER ){
		/*printf("id:%d name:%s\n",
			   ((req_conn_peer*)buf)->peer_id, 
			   ((req_conn_peer*)buf)->peer_name );*/
		
		printf(">Accetti connessione con %s? [S/N]: ", ((req_conn_peer*)buf)->peer_name);
		scanf(" %[^\n]c",&c);
		//scanf("%c",&c);
		if( c=='S' || c=='s' ) {
			//printf("Accettato\n");
			re.t = ACCEPT_CONN_FROM_PEER;
		} else {
			//printf("Rifiutato\n");
			re.t = REFUSE_CONN_FROM_PEER;
		}
		re.sender_id = ((req_conn_peer*)buf)->peer_id;
		re.receiver_id = my_id;
		//convert_to_network_order(&re);
		send_data(sockt,(char*)&re,sizeof(re));	
		
		if( c=='S' || c=='s')
			switch_mode();		
	} else if ( m_type == CONN_TO_PEER_ACCEPTED ) { 
		/*il server invia erronamente un messaggio
		 *di connessione accettata mentre si e' in
		 *partita.*/
		if( mode == '#') return;

		/*Copio i dati dell'avversario presenti nel messaggio
		 *nelle variabili globali*/
		strcpy(opposite_name,((req_conn_peer*)buf)->peer_name);
		opposite_udp_port = ((req_conn_peer*)buf)->peer_udp_port;
		memcpy((void*)&opposite_addr, (void*)&(((req_conn_peer*)buf)->peer_addr), sizeof(opposite_addr));

		printf("Richiesta di connessione accettata\n");
		switch_mode();
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
	}
}

void disconnect()
{
	#ifdef DEBUG
	printf("disconnessione\n");
	#endif
	close(sock_serv_TCP);
	exit(0);
}

void switch_mode(){
	if( mode == '>' ) mode = '#';
	else mode = '>';
}