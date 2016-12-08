#include "game.h"
#include "TCP.h"
#include "messages.h"

#define DEBUG

int my_id = -1;
uint16_t my_udp_port;
char my_name[64];
int sock_serv_TCP;
const unsigned short DIM_BUFF = 512;
char mode = '>';


void help();
void register_to_serv();
void execute_cmd(char* str, int sockt);
void remote_req(int sockt, char* buf);
void disconnect();
void switch_mode();

void help()
{
	printf("\n");
	printf("Sono disponibili i seguenti comandi:\n");
	printf("   !help --> mostra l'elenco dei comandi disponibili\n");
	printf("   !who --> mostra l'elenco dei client connessi al server\n");
	printf("   !connect username --> avvia una partita con l'utente username\n");
	printf("   !quit --> disconnette il client dal server\n\n");
}

void register_to_serv( int sd )
{
	char buf[DIM_BUFF];
	char* buf_rec = NULL;
	uint32_t len;
	
	reg_set_name r = INIT_REG_SET_NAME;
	
	int ret = recv_data(sd, &buf_rec);
	sscanf(buf_rec, "WELCOME YOUR ID IS %d", &my_id);
	printf(">MY ID IS: %d \n",my_id);

	for(;;) 
	{
		printf(">Inserisci il tuo nome: ");
		scanf("%s", buf);
		if( strlen(buf) > 63 )
			printf(">Il tuo nome e' troppo lungo e verra' troncato");
		buf[63] = '\0';

		r.t = htonl(PEER_SETS_NAME);
		strcpy(r.name,buf);

		ret = send_data( sd, (char*)&r, sizeof(r) );
		ret = recv_data(sd, &buf_rec); 
	//	printf("ricevuto %s \n",buf);
		/*NAME SET
	     *NAME ALREADY USED
	     */
		/*if( strcmp( buf_rec, "NAME SET") == 0 ) {
			printf(">Nome accettato\n");
			break;*/

		message_type m;
		memcpy(&m, buf_rec, sizeof(m));

		if( m == NAME_ACCEPTED ){
			printf(">Nome accettato\n");
			break;
		} else
			printf(">Il nome e' gia' usato da un altro peer. \n");
	}

	printf(">Inserisci la tua porta UDP: ");
	scanf("%hu",&my_udp_port);
	my_udp_port = htons(my_udp_port);

	sprintf(buf, "MY PORT IS %hu",my_udp_port);
	send_data( sd, buf, strlen(buf)+1 );

	free(buf_rec);
}

void execute_cmd(char* str, int sockt)
{
	char lower_str[10];
	char higher_str[65];
	char* buf = NULL;

	//printf("execute_cmd...\n");

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
		sprintf(str,"CONNECT %s",higher_str);			
		send_data(sockt,str,strlen(str)+1);
		//recv_data(sockt,&buf);
	}

	if( buf )
		free(buf);
}

void remote_req( int sockt, char* buf )
{
	message_type m_type = GENERIC_ERR;
	memcpy((void*)&m_type, (void*)buf, sizeof(m_type));
	m_type = ntohl(m_type);

	if( m_type == REQ_CONN_TO_PEER ){
		printf("Richiesta di connessione\n");
		printf("id:%d name:%s\n",
			   ((req_conn_to_peer*)buf)->sender_id, 
			   ((req_conn_to_peer*)buf)->sender_name );
	} else {
		printf("altra richiesta\n");
	}

}


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

	FD_SET(0,&master);	//stdin
	FD_SET(sd,&master);

	while(1) {
		printf("%c ",mode);
		fflush(stdout);
		read_fds = master;
		select( sd+1, &read_fds, NULL, NULL, NULL);

		if( FD_ISSET(sd, &read_fds) )
		{
			recv_data(sd,&buf_rec);
			//printf("%s\n",buf_rec);
			remote_req(sd, buf_rec);
		} else {
			scanf(" %[^\n]s",buf);
			execute_cmd(buf,sd);
		}
	}

	close(sd);
	return 0;
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