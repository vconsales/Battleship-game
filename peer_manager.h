#ifndef PEER_MANAGER_H
#define PEER_MANAGER_H

#include "TCP.h"

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
	char name[64];
	uint16_t udp_port; /*porta(big endian) sulla quale il peer accetta connessioni da altri peer.*/
	peer_state state;
}des_peer;

/**attenzione all'esterno si puo' mettere in stato inconsistente**/
extern des_peer** peers; 

int get_index_peer( int sockt );
int add_peer( int n_peers_connected );
int remove_peer_having_sock( int sockt );
int get_max_peers();
//int more_peers(); aumenta il numero di peer massimi

#endif