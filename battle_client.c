#include "game.h"
#include "TCP.h"

#define DIM_BUFF 1024

int my_id = -1;
uint16_t my_udp_port;
char my_name[64];

void register_to_serv( int sd )
{
	char buf[DIM_BUFF];
	uint32_t len, dim_len = sizeof(uint32_t);
	uint32_t no_len; //lunghezza in formato network

	//int ret = recv( sd, buf, 30, MSG_WAITALL );
	int ret = recv_data(sd, buf, 100);
	sscanf(buf, "WELCOME YOUR ID IS %d", &my_id);
	printf(">MY ID IS: %d \n",my_id);

	for(;;) {
		printf(">Inserisci il tuo nome: ");
		scanf("%s", buf);
		if( strlen(buf) > 63 )
			printf(">Il tuo nome e' troppo lungo e verra' troncato");

		strncpy ( my_name, buf, 63 );
		my_name[64] = '\0';

		memset(buf,0,DIM_BUFF);
		sprintf(buf, "MY NAME IS %s",my_name);
	
		len = strlen(buf)+1;

		printf("mando la stringa %s di lunghezza %d\n",buf,len);
		ret = send_data( sd, buf, len );
		ret = recv_data(sd, buf, 18); //da cambiare
		printf("ricevuto %s \n",buf);
		/*NAME SET
	     *NAME ALREADY USED
	     */
		if( strcmp( buf, "NAME SET") == 0 ){
			printf(">Nome accettato\n");
			break;
		}
		else
			printf(">Il nome e' gia' usato da un altro peer. \n");
	}

	printf(">Inserisci la tua porta UDP: ");
	scanf("%hu",&my_udp_port);
	my_udp_port = htons(my_udp_port);

	sprintf(buf, "MY PORT IS %hu",my_udp_port);
	send_data( sd, buf, strlen(buf)+1 );
}

int main(int argc, char* argv[])
{
	int sd, ret;
	struct sockaddr_in serv_addr;
	char buf[DIM_BUFF];

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
	
	sleep(10);
	close(sd);
	return 0;
}