typedef enum { 
	WELCOME_MESS,
	PEER_SETS_NAME,
	PEER_SETS_UDP_PORT,
	NAME_ACCEPTED,
	NAME_REFUSED,
	REGISTERED,
	LIST_OF_PEERS,
	REQ_CONN_TO_PEER,
	REQ_CONN_FROM_PEER,
	PEER_DOES_NOT_EXIST,
	CONN_TO_PEER_ACCEPTED,
	CONN_TO_PEER_REFUSED,
	ACCEPT_CONN_FROM_PEER,
	REFUSE_CONN_FROM_PEER,
	PEER_IS_NOT_FREE,
	GENERIC_ERR,
	DISCONNECT,
} message_type;

#define INIT_REG_SET_NAME { PEER_SETS_NAME, -1, {"\0"} }
#define INIT_REG_SET_UDP_PORT { PEER_SETS_UDP_PORT, 0 }
#define INIT_REQ_CONN_FROM_PEER { REQ_CONN_FROM_PEER, -1, '\0' }
/*#define INIT_ACCEPT_CONN_TO_PEER { ACCEPT_CONN_PEER, -1, -1 }
#define INIT_REFUSE_CONN_TO_PEER { REFUSE_CONN_PEER, -1, -1 }
*/

typedef struct reg_set_name_t
{
	message_type t;
	int peer_id;
	char name[65];
}__attribute__((packed)) reg_set_name;

typedef struct reg_set_udp_port_t
{
	message_type t;
	int peer_id;
	uint16_t udp_port;
}__attribute__((packed))reg_set_udp_port;


/*******************************************
* Questo messaggio viene inviato  per gestire 
* le richieste di connessione da un peer ad
* un altro per iniziare una partita.
* Il messaggio ha nel campo t:
* - REQ_CONN_TO_PEER nel caso in cui il 
*   peer invia al server una richiesta di
*   connessione verso un altro peer.
* - REQ_CONN_FROM_PEER nel caso in cui il 
*   server inoltra al peer destinatario una
*   richiesta di connessione.
* - CONN_TO_PEER_ACCEPTED nel caso in cui
*   il server risponde al mittente che la
*   sua richiesta di connessione e' stata
*   accettata.
* - CONN_TO_PEER_REFUSED nel caso in cui
*   il sever risponde al mittente cha la
*   sua richiesta di connessione e' stata 
*   rifiutata.
* - PEER_DOESNT_EXIST nel caso in cui il
*   server non trova un peer con il nome
*   indicato.
* - PEER_IS_NOT_FREE nel caso in cui il
*   peer indicato dal mittente sta gia'
*   giocando con un altro peer. 	
* Nel primo caso il campo id identifica il
* peer mittente mentre il campo name indica
* il nome del destinatario.
* Nel secondo caso i campi id,name,udp_port,
* addr identificano il peer mittente.
* Nel terzo caso i campi id,name,udp_port,
* addr identificano il peer destinatario.
* Negli altri casi i campi non sono
* significativi.
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
* t vale ACCEPT_CONN_FROM_PEER se il ricevitore
* della richiesta accetta la connessione.
* t vale REFUSE_CONN_FROM_PEER se il ricevitore
* della richiesta rifiuta la connessione.
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