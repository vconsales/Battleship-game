#ifndef PEER_MANAGER_H
#define PEER_MANAGER_H
#include "net_wrapper.h"

#define NAME_LEN 64
//#define DEBUG
typedef enum { UNSET, NAME_SET, PEER_FREE, PEER_PLAYING } peer_state;

/***************Descrittore di peer.*******************
 *Contiene informazioni che caratterizzano il client
 *quali il suo indirizzo ip, la socket con la quale
 *il server e' connesso ad esso, la porta udp su cui
 *il client(o peer) ascolta gli altri peer e lo stato
 *in cui esso si trova.
 *****************************************************/
typedef struct des_peer_t 
{
	ConnectionTCP conn;
	char name[NAME_LEN];
	uint16_t udp_port; /*porta(big endian) sulla quale il peer accetta connessioni da altri peer.*/
	uint16_t opponent_id; /*id dell'avversario*/
	peer_state state;
}des_peer;

int init_peer_manager();
des_peer* get_peer( int index );
int remove_peer(int index );
int get_index_peer_name( char* name );
int get_index_peer_sock( int sockt );
int add_peer();
int remove_peer( int index );
int get_n_peers();

/*se l'id corrisponde ad un peer registrato ritorna 1 altrimenti 0*/
int is_valid_id( int id );

/*
* Fornire come primo parametro l'indirizzo di un puntatore
* a caratteri (NULL) e come secondo parametro l'indirizzo
* di un puntatore a uint8_t.
* La funzione alloca lo spazio necessario ed inserisce in
* in list_name solo i nomi dei peer registrati. Nel vettore
* state inserisce lo stato del peer, ovvero:
* 0->libero
* 1->occupato
* La restituisce il numero di peer trovati.
*/
int get_peers_registred( char** list_name, uint8_t** state );
int more_peers();// aumenta il numero di peer massimi

#endif
