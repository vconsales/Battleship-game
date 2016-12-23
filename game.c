/*
*!game
*/
#include "game.h"

/* bit map variabile state
7 -> locale/remota
6 -> nessun significato
5 -> navi affondate
4 -> navi affondate
3 -> navi affondate
2 -> navi posizionate
1 -> navi posizionate
0 -> navi posizionate
*/

const uint8_t b_LOCAL =(uint8_t) 1 << 7;
const uint8_t b_HIT = (uint8_t) 7 << 3;
const uint8_t b_POS = (uint8_t) 7;
const char c_MISS = '0';
const char c_HIT = '!';
const char c_EMPTY = ' ';
const char c_SHIP = 'X';


static int normalize( coordinate *co );
static void set_n_ship_hit( battle_game *bg, uint8_t n);
static void set_n_ship_pos( battle_game *bg, uint8_t n);
static int shot_ship_local( coordinate *co, battle_game* bg );
static int shot_ship_remote( coordinate *co, battle_game *bg );
static void send_ship_arranged( int sock_udp );
static void send_ship_hit( int sock_udp, char col, char row );
static void send_ship_miss( int sock_udp, char col, char row );
static void send_you_won( int sock_udp );

uint8_t is_local(battle_game *bg)
{
	return (bg->state & b_LOCAL);
}

uint8_t is_remote(battle_game *bg)
{
	return !(bg->state & b_LOCAL);
}
/*debug*/
void print_bitmap()
{
	printf("%d %d %d \n",b_LOCAL, b_HIT, b_POS);
}

void show_grind( battle_game *bg_l, battle_game *bg_r )
{
	short i, j;

	/*if( is_local(bg) )
		printf("GRIGLIA LOCALE\n");
	else
		printf("GRIGLIA REMOTA\n");*/

	printf("   A B C D E F ");
	if( bg_r )
		printf("\t   A B C D E F \n");
	else
		printf("\n");

	printf("  +-----------+");

	if( bg_r ){
		printf("\t  +-----------+\n");
	} else
		printf("\n");

	for(i=0; i<SIZE_GRIND; i++){
		//printf("+-----------------+\n");
		printf("%d ", i+1);
		for(j=0; j<SIZE_GRIND; j++){
			printf("|%c",bg_l->battle_grind[i][j]);
		}
		if( bg_r ) {
			printf("|\t  ");
			fflush(stdout);
			for(j=0; j<SIZE_GRIND; j++){
				printf("|%c",bg_r->battle_grind[i][j]);
			}
		}
		printf("|\n");
	}

	printf("  +-----------+");
	if( bg_r ) {
		printf("\t  +-----------+\n");
	}
	else
		printf("\n");
}

int init_game( battle_game *bg, int local, int sockt )
{
	if( bg == NULL )
		return -1;

	bg->state = 0;

	if( local )
		bg->state |= b_LOCAL;

	bg->sock_udp = sockt;

	memset( bg->battle_grind, ' ', sizeof(char)*SIZE_GRIND*SIZE_GRIND);

	return 0;
}

int init_local_game( battle_game *bg )
{
	return init_game(bg,1,0);
}
int init_remote_game( battle_game *bg, int sockt)
{
	return init_game(bg, 0, sockt);
}

int set_ship( coordinate *c, battle_game *bg )
{
	int n_pos = get_n_ship_pos(bg);	

	if( !is_local(bg) )
		return -3;

	if( n_pos >= SHIP_NUMBER )
		return -4;

	int res = normalize(c); 
	if( res == -1 )
		return -1;

	if( bg->battle_grind[c->y][c->x] == c_SHIP )
		return -2;
	
	/*Imposto la nave nella posizione*/
	bg->battle_grind[c->y][c->x] = c_SHIP;
	/*Incremento il numero di navi*/
	set_n_ship_pos(bg,++n_pos);

	if( get_n_ship_pos(bg) == SHIP_NUMBER ) {
		send_ship_arranged(bg->sock_udp);
		return 1;
	}

	return 0;
}

int shot_ship( coordinate *c, battle_game *bg )
{
	if( is_local(bg) )
		return shot_ship_local(c,bg);
	else
		return shot_ship_remote(c,bg);
}

static
int normalize( coordinate *co )
{
	if( co->x >= 97)
		co->x = (uint8_t)co->x - 'a';
	else	
		co->x = (uint8_t)co->x - 'A';
	co->y = (uint8_t)co->y - '0' - 1;
	
	#ifdef DEBUG
	printf("co->x %d co->y %d\n",co->x,co->y);
	#endif

	if( co->x<0 || co->y<0 || co->x>=SIZE_GRIND || co->y>=SIZE_GRIND )
		return -1;
	else
		return 0;
}

static
int shot_ship_local( coordinate* c, battle_game *bg )
{
	uint8_t n_hit = get_n_ship_hit(bg);
	int ret = normalize(c); 
	
	/*Le coordinate passate non sono valide*/
	if( ret == -1 )
		return -1;

	/*Se nella casella selezionata c'è una nave...*/
	if( bg->battle_grind[c->y][c->x] == c_SHIP ){
		bg->battle_grind[c->y][c->x] = c_HIT; /*Nave colpita*/
		set_n_ship_hit(bg, ++n_hit); /*Aggiorno n. navi colpite*/
		send_ship_hit(bg->sock_udp,c->x,c->y);

		if( n_hit == SHIP_NUMBER){
			send_you_won(bg->sock_udp);
			return 2; /*Gioco terminato*/
		}
		else
			return 1;
	}

	/*Se la casella è vuota...*/
	if( bg->battle_grind[c->y][c->x] == c_EMPTY ){
		bg->battle_grind[c->y][c->x] = c_MISS;
		send_ship_miss(bg->sock_udp,c->x,c->y);
		return 0;
	} else
		return -2; /*Nave già colpita*/
}

static
int shot_ship_remote( coordinate *co, battle_game *bg )
{
	int ret;
	shot_mess sm;
	sm.t = SHOT_SHIP;
	sm.col = co->x;
	sm.row = co->y;

	ret = normalize(co);	
	/*le coordinate passate non sono valide*/
	if( ret == -1 )
		return -1;

	/*se il colpo in quella casella è stato già sparato
	  non ha senso mandare di nuovo il pacchetto.*/
	if( bg->battle_grind[co->y][co->x] != c_EMPTY )
		return -2;

	#ifdef DEBUG
	printf("col:%c row:%c\n",co->x,co->y);
	#endif
	send_data(bg->sock_udp,(char*)&sm,sizeof(sm));

	return 0;
}

static
void set_n_ship_hit( battle_game *bg, uint8_t n )
{
	/*azzera le navi colpite*/
	uint8_t mask = 0xFF ^ b_HIT; 
	bg->state &= mask;

	/*setta le navi colpite*/
	bg->state |= (n << 3);
}

static
void set_n_ship_pos( battle_game *bg, uint8_t n ) 
{
	/*azzera le navi posizionate*/
	uint8_t mask = 0xFF ^ b_POS; 
	bg->state &= mask;

	/*setta le navi posizionate*/
	bg->state |= n ;
}

uint8_t get_n_ship_hit( battle_game *bg )
{
	uint8_t mask_hit = 7 << 3;
	uint8_t res = bg->state & mask_hit;
	return (res >> 3);
}

uint8_t get_n_ship_pos( battle_game *bg )
{
	return bg->state & b_POS;
}

int set_hit( coordinate* co, battle_game* bg_r )
{
	if( co->x<0 || co->y<0 || co->x>=SIZE_GRIND || co->y>=SIZE_GRIND )
		return -1;

	int n_ship_hit = get_n_ship_hit(bg_r)+1;
	set_n_ship_hit(bg_r,n_ship_hit);

	bg_r->battle_grind[co->y][co->x] = c_HIT;
	return 0;
}

int set_miss( coordinate* co, battle_game* bg_r )
{
	if( co->x<0 || co->y<0 || co->x>=SIZE_GRIND || co->y>=SIZE_GRIND )
		return -1;

	bg_r->battle_grind[co->y][co->x] = c_MISS;
	return 0;
}

static
void send_ship_arranged( int sock_udp )
{
	message_type mt = SHIP_ARRANGED;
	send_data(sock_udp, (char*)&mt, sizeof(mt));
}

static
void send_you_won( int sock_udp )
{
	message_type mt = YOU_WON;
	send_data(sock_udp, (char*)&mt, sizeof(mt));
}

static
void send_ship_hit( int sock_udp, char col, char row )
{
	shot_mess sm = INIT_SHIP_HIT(col,row);
	//printf("mando HIT %c %c",col,row);
	send_data(sock_udp,(char*)&sm,sizeof(sm));
}

static
void send_ship_miss( int sock_udp, char col, char row )
{
	shot_mess sm = INIT_SHIP_MISS(col,row);
	//printf("mando MISS %c %c",col,row);
	send_data(sock_udp,(char*)&sm,sizeof(sm));
}