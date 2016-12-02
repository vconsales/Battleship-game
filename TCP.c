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
//	printf("nuovo peer connesso. nuova socket: %d \n",sd);

	return sd;
}

int recv_data( int sockt, char* buf, uint32_t buf_len )
{
	uint32_t nbytes = 0, len_nbytes = sizeof(uint32_t);
	int received = 0; 
	

//	printf("ricevo dim pacchetto su socket %d\n",sockt);
	received = recv( sockt, (void*)&nbytes, len_nbytes, MSG_WAITALL );

	if( received < len_nbytes )
	{
		//printf("pacchetto {nbytes} ha dim %d \n", received);
		//my_errno = N_BYTE_NOT_DEFINED;
		return -1;
	}

	nbytes = ntohl(nbytes);
	nbytes = (nbytes < buf_len) ? nbytes : buf_len;
	//printf("Byte da ricevere %d\n", nbytes);	

	received = recv( sockt, buf, nbytes, MSG_WAITALL );
//	printf("received:%d buf: %s\n",received,buf);
	if( received != nbytes ){
		//printf("pacchetto {buf} ha dim %d \n", received);
		my_errno = LESS_BYTE_RECEIVED;
		return -1;
	}

	return received;
}

int send_data( int sockt, char* buf, uint32_t buf_len )
{
	uint32_t nbytes = htonl(buf_len), len_nbytes = sizeof(uint32_t);
	int bsend = 0;

	bsend = send(sockt, (void*)&nbytes, len_nbytes, 0);

	if( bsend < len_nbytes )
	{
		printf("pacchetto {nbytes} ha dim %d \n", bsend);
		return -1;
	}

	bsend = send(sockt, (void*)buf, buf_len, 0);

	if( bsend < buf_len )
		printf("pacchetto {buf} ha dim %d, inviati %d ",buf_len,bsend);
	

	return bsend;
}