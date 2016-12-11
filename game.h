#ifndef GAME_H
#define GAME_H

#include "TCP.h"
#include "messages.h"

#define SIZE_GRIND 6
#define SHIP_NUMBER 7
#define DEBUG
/****
* per le griglie locali
* x ->nave posizionata
* 0 -> colpo a vuoto
* ' ' -> zona vuota/non sparata
* ! -> nave colpita
*****/


/**
*Per le griglie remote
* 0 -> colpo a vuoto
* ! -> nave colpita
* ' ' -> zona non sparata
*/

/*******************************************************
* Struttura dati che rappresenta una griglia del gioco
* Il campo state mantiene tali informazioni:
* -griglia remota o locale
* -il numero di navi posizionate
* -il numero di navi affondate
* Il campo sock_udp indica(in qualunque caso) il socket
* udp "connesso" all'avversario.
* Non modificare esternamente il campo state per non 
* lasciare la griglia in uno stato inconsistente.
*******************************************************/
typedef struct battle_game_t
{
	int sock_udp; 
	char battle_grind[SIZE_GRIND][SIZE_GRIND];
	uint8_t state;
}battle_game;

typedef struct coord_t
{
	unsigned char x;
	unsigned char y;
}coordinate;

int init_game( battle_game *bg, int local, int sockt );
int init_local_game( battle_game *bg );
int init_remote_game( battle_game *bg, int sockt);
void show_grind( battle_game *bg );

/*******************************************
* Imposta la nave nelle coordinate indicate
*  0 se l'operazione e' avvenuta con successo 
* -1 se le coordinate non sono valide
* -2 se il posto e' occupato
* -3 se la griglia non e' locale
* -4 se tutte le navi sono state posizionate
********************************************/
int set_ship( coordinate *c, battle_game *bg );

/******************************************
* Spara il colpo nelle coordinate indicate.
* Se la griglia e' remota invia il messaggio
* su sock_udp.
* Se la griglia e' locale modifica la griglia
* ed invia un messaggio di HIT/MISS a sock_udp.
* Ignorare i result a seguire...
*  2 tutte le navi sono state affondate
*  1 se la nave e' stata colpita
*  0 se il colpo e' andato a vuoto
* -1 se le coordinate non sono valide
* -2 se il colpo e' stato gia' sparato 
* -3 impossibile contattare il peer
***************************************/
int shot_ship( coordinate* c, battle_game* bg );

/************************************************
* Usato per modificare lo stato di una griglia 
* remota quando il peer riceve un messaggio di
* MISS. Restituisce il numero di navi colpite.
* Se le coordinate non sono valide ritorna -1.
************************************************/
int set_miss( coordinate* c, battle_game* bg_r );

/************************************************
* Usato per modificare lo stato di una griglia 
* remota quando il peer riceve un messaggio di
* HIT. Restituisce il numero di navi colpite.
* Se le coordinate non sono valide ritorna -1.
************************************************/
int set_hit( coordinate* c, battle_game* bg_r );

uint8_t is_local( battle_game *bg );
uint8_t is_remote( battle_game *bg );
uint8_t get_n_ship_hit( battle_game *bg );
uint8_t get_n_ship_pos( battle_game *bg );
void print_bitmap(); /*debug*/

#endif
