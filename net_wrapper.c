#include "net_wrapper.h"

int open_serverTCP( uint16_t port )
{
	char ip_str[16];
	int sock = -1, ret=-1;
	struct sockaddr_in my_addr;

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
	printf("Server aperto. ip:%s porta:%hu\n",ip_str,port);

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

int send_data( int sockt, char* buf, uint32_t buf_len )
{
	uint32_t nbytes = htonl(buf_len);
	uint32_t bsend = 0;

      /*invia quanti byte contiene il messaggio(formato big endian)*/
	bsend = send(sockt, (void*)&nbytes, 4, 0);
      
      #ifdef DEBUG_NET_WRAPPER	
      printf("pacchetto nbytes mandati %ld B e vale %ld",bsend,len_nbytes);
      #endif

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

int recv_data( int sockt, my_buffer* my_buff )
{
	uint32_t nbytes = 0;
	uint32_t received = 0; 

	received = recv( sockt, (void*)&nbytes, 4, 0 );

	if( received < 4 )
		return -1;
	
	nbytes = ntohl(nbytes);
	if( nbytes > my_buff->size )
	{
		if( my_buff->buf != NULL )
			free(my_buff->buf);
		my_buff->buf = (char*)malloc(nbytes);	
		my_buff->size = nbytes;
	}
	clear_my_buffer(my_buff);

	received = recv( sockt, my_buff->buf, nbytes, 0 );
	
	if( received != nbytes )
		return -1;

	return received;
}
void clear_my_buffer( my_buffer *myb )
{
	if( myb->size > 0 && myb->buf!=NULL )
		memset(myb->buf,0,myb->size);
}