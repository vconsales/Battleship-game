#include "net_wrapper.h"

int open_serverTCP( uint16_t port )
{
	char ip_str[16];
	int sock = -1, ret=-1;
	struct sockaddr_in my_addr;

	//ServerTCP *new_serv = (ServerTCP*) malloc(sizeof(ServerTCP));	
	ret = socket(AF_INET, SOCK_STREAM, 0);

	if( ret == -1 ) {
		perror("Impossibile aprire socket");
		return -1;
	}
	sock = ret;

	memset(&my_addr, 0, sizeof(struct sockaddr_in));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sock, (struct sockaddr*)&my_addr, sizeof(struct sockaddr_in));
	
	if( ret == -1 )
	{
		//*printf("bind fallita \n");
		perror("Bind fallita");
		close(sock);
		return -1;
	}

	inet_ntop(AF_INET, &my_addr.sin_addr, ip_str, 16);
	#ifdef DEBUG
	printf("Server aperto. ip:%s porta:%hu socket:%d\n",ip_str,port,sock_serv);
	#else
	printf("Server aperto. ip:127.0.0.1 porta:%hu\n",port);
	#endif

	ret = listen(sock, 10);
	
	if( ret == -1)
		return -1;
	else
		return sock;
}

int close_connection( ConnectionTCP *conn )
{
	return close(conn->socket);
}


int accept_serverTCP( int sock_serv, ConnectionTCP *conn )
{
	int sd;
	char ip_str[16];
	socklen_t len = sizeof(conn->cl_addr);
	/*printf("sto per accettare. len=%d \n",len);*/
	sd = accept(sock_serv, (struct sockaddr*)&conn->cl_addr, &len);
	if( sd == -1 ) {
		perror("Errore accept");
		return -1;
	}
	conn->socket = sd;

	inet_ntop(AF_INET, ((char*)&conn->cl_addr.sin_addr), ip_str, 16);
	printf("nuovo peer connesso ip:%s porta:%hu\n",ip_str,ntohs(conn->cl_addr.sin_port));
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
		return -1;
	}

	return received;
}

int send_data( int sockt, char* buf, uint32_t buf_len )
{
	uint32_t nbytes = htonl(buf_len);
	uint32_t bsend = 0;

	bsend = send(sockt, (void*)&nbytes, 4, 0);
	//printf("pacchetto nbytes mandati %ld B e vale %ld",bsend,len_nbytes);

	if( bsend < 4 )
	{
		printf("pacchetto {nbytes} ha dim %d \n", bsend);
		return -1;
	}

	bsend = send(sockt, (void*)buf, buf_len, 0);
	//sprintf("dim=%d mess=%s inviati=%d\n",buf_len,buf,bsend);

	if( bsend < buf_len )
		printf("pacchetto {buf} ha dim %d, inviati %d ",buf_len,bsend);
	

	return bsend;
}