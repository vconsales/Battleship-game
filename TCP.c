#include "TCP.h"

int open_serverTCP( uint16_t port, ServerTCP **serv )
{
	int ret = -1;

	ServerTCP *new_serv = (ServerTCP*) malloc(sizeof(ServerTCP));	
	ret = socket(AF_INET, SOCK_STREAM, 0);

	if( ret == -1 )
		return ret;

	new_serv->socket = ret;

	memset( &new_serv->my_addr, 0, sizeof(struct sockaddr_in));
	new_serv->my_addr.sin_family = AF_INET;
	new_serv->my_addr.sin_port = htons(port);
	new_serv->my_addr.sin_addr.s_addr = INADDR_ANY;
	new_serv->peers_connected = 0;

	ret = bind( new_serv->socket, (struct sockaddr*)&new_serv->my_addr, sizeof(struct sockaddr_in) );
	
	if( ret == -1 )
	{
		printf("bind fallita \n");
		close( new_serv->socket );
		return ret;
		/*exit(1);*/
	}

	ret = listen( new_serv->socket, 10);

	*serv = new_serv;

	return ret;
}

int close_serverTCP( ServerTCP **serv )
{
	int ret = close( (*serv)->socket );
	free( *serv );
	*serv = NULL;
	return ret;	
}

int close_connection( ConnectionTCP *conn )
{
	return close(conn->socket);
}


int accept_serverTCP( ServerTCP *serv, ConnectionTCP *conn )
{
	int sd;
	socklen_t len = sizeof(conn->cl_addr);
	/*printf("sto per accettare. len=%d \n",len);*/
	sd = accept(serv->socket, (struct sockaddr*)&conn->cl_addr, &len );
	conn->socket = sd;
	serv->peers_connected++;
	printf("nuovo peer connesso. nuova socket: %d \n",sd);

	return sd;
}

//gestire la conversione di formato
int recv_data( int sockt, char** buf )
{
	if( *buf != NULL )
	{
		free(*buf);
		*buf = NULL;
	}

	uint32_t nbytes = 0;
	uint32_t received = 0; 

//	printf("ricevo dim pacchetto su socket %d\n",sockt);
	received = recv( sockt, (void*)&nbytes, 4, 0 );
	//printf("pacchetto nbytes ha dim %ld e vale %ld\n",received,nbytes);

	if( received < 4 )
	{
		//printf("pacchetto {nbytes} ha dim %d \n", received);
		//my_errno = N_BYTE_NOT_DEFINED;
		return -1;
	}

	nbytes = ntohl(nbytes);
	*buf = (char*)malloc(nbytes);	
//	printf("Byte {prova} vale %ld buf_len vale %ld\n", prova,buf_len);	

	received = recv( sockt, *buf, nbytes, 0 );
	//printf("received:%d buf: %s\n",received,*buf);
	if( received != nbytes ){
		//printf("pacchetto {buf} ha dim %d \n", received);
		my_errno = LESS_BYTE_RECEIVED;
		return -1;
	}

	return received;
}

int send_data( int sockt, char* buf, uint32_t buf_len )
{
	uint32_t nbytes = htonl(buf_len);
	uint32_t bsend = 0;

	bsend = send(sockt, (void*)&nbytes, 4, 0);
//	printf("pacchetto nbytes mandati %ld B e vale %ld",bsend,len_nbytes);

	if( bsend < 4 )
	{
		printf("pacchetto {nbytes} ha dim %d \n", bsend);
		return -1;
	}


//	printf("dim=%d mess=%s\n",buf_len,buf);

	bsend = send(sockt, (void*)buf, buf_len, 0);

	if( bsend < buf_len )
		printf("pacchetto {buf} ha dim %d, inviati %d ",buf_len,bsend);
	

	return bsend;
}