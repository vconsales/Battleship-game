#ifndef PEER_MANAGER_H
#define PEER_MANAGER_H
#include "TCP.h"

#define NAME_LEN 64

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

/*da implementare per rendere più sicuro il tutto*/
typedef struct peer_vector_t
{
	des_peer** peers;
	unsigned int n_peer;
}peer_vector;

/**attenzione all'esterno si puo' mettere in stato inconsistente**/
extern des_peer** peers; 

int get_index_peer_name( char* name );
int get_index_peer_sock( int sockt );
int add_peer( int n_peers_connected );
int remove_peer_having_sock( int sockt );
int get_max_peers();

/*se l'id corrisponde ad un peer registrato ritorna 1 altrimenti 0*/
int is_valid_id( int id );

/*Fornire come primo parametro una array di puntatori 
* a carattere di dimensione n_peers.
*/
int get_peers_name( char** list, int n_peers );
int more_peers();// aumenta il numero di peer massimi

#endif