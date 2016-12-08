typedef enum { 
	WELCOME_MESS,
	PEER_SETS_NAME,
	PEER_SETS_UDP_PORT,
	NAME_ACCEPTED,
	NAME_REFUSED,
	REGISTERED,
	LIST_OF_PEERS,
	REQ_CONN_FROM_PEER,
	PEER_DOESNT_EXIST,
	REQ_CONN_ACCEPTED,
	REQ_CONN_REFUSED,
	ACCEPT_CONN_PEER,
	REFUSE_CONN_PEER,
	GENERIC_ERR,
	DISCONNECT,
} message_type;

#define INIT_REG_SET_NAME { PEER_SETS_NAME, {"\0"} }
#define INIT_REG_SET_UDP_PORT { PEER_SETS_UDP_PORT, 0 }
#define INIT_REQ_CONN_FROM_PEER { REQ_CONN_FROM_PEER, -1, '\0' }
#define INIT_ACCEPT_CONN_TO_PEER { ACCEPT_CONN_PEER, -1, -1 }
#define INIT_REFUSE_CONN_TO_PEER { REFUSE_CONN_PEER, -1, -1 }


typedef struct reg_set_name_t
{
	message_type t;
	char name[65];
}__attribute__((packed)) reg_set_name;

typedef struct reg_set_udp_port_t
{
	message_type t;
	uint16_t udp_port;
}__attribute__((packed))reg_set_udp_port;


/*******************************************
* Questo messaggio viene inviato dal server
* ad un peer per gestire le richieste di
* connessione.
* Il messaggio ha nel campo t:
* - REQ_CONN_FROM_PEER nel caso in cui il 
*   server inoltra al peer destinatario una
*   richiesta di connessione.
* - REQ_CONN_ACCEPTED nel caso in cui il
*   server risponde al mittente che la sua
*   richiesta di connessione e' stata
*   accettata.
* - REQ_CONN_REFUSED nel caso in cui il
*   sever risponde al mittente cha la sua
*   richiesta di connessione e' stata 
*   rifiutata.
* - PEER_DOESNT_EXIST nel caso in cui il
*   server non trova un peer con il nome
*   indicato. 	
* Nel primo caso i campi name,udp_port,addr
* identificano il peer mittente.
* Nel secondo caso i campi name,udp_port,
* addr identificano il peer destinatario.
* Nel terzo e nel quarto caso i campi non
* sono significativi.
*******************************************/
typedef struct req_conn_peer_t
{
	message_type t;
	int peer_id;
	char peer_name[65];
	uint16_t peer_udp_port;
	struct sockaddr_in peer_addr; 
}__attribute__((packed)) req_conn_peer;


/*******************************************
* Questo messaggio viene inviato da un peer
* al server in risposta ad una richiesta di
* connessione. 
* t vale ACCEPT_CONN_PEER se il ricevitore
* della richiesta accetta la connessione
* t vale REFUSE_CONN_PEER se il ricevitore
* della richiesta rifiuta la connessione
* I due id sono utilizzati per identificare
* i peer in modo efficiente.
********************************************/
typedef struct response_conn_to_peer_t
{
	message_type t;
	int sender_id;
	int receiver_id;
}__attribute__((packed)) response_conn_to_peer;

/*******************************************
* Esempio:
* Il peer A vuole giocare con B
* A invia un messaggio req_conn_to_peer al
* server indicando nei campi   
*
********************************************/
/*gestire il network order*/
void convert_to_network_order( void* msg )
{

}

void convert_to_host_order( void* msg )
{

}