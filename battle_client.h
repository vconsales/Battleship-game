/***Variabili globali***/
my_buffer my_buf = INIT_MY_BUFFER;
const time_t N_SECONDS = 60;

int sock_serv_TCP;
int my_id = -1;
uint16_t my_udp_port;
char my_name[64];
const unsigned short DIM_BUFF = 512;
struct sockaddr_in my_addr;
battle_game my_grind;

int socket_udp;
char opposite_name[64];
struct sockaddr_in opposite_addr;
battle_game opposite_grind;
char mode = '>';

unsigned char state = 0;
/*************************************/

/*
 *bit map della variabile state
 * Nel caso in cui il bit n.x sia settato
 * vale ciò che è scritto dopo -> 
 * 7 -> partita in corso
 * 6 -> l'avversario è pronto per giocare  
 * 5 -> sto posizionando le navi
 * 4 -> è il tuo turno in game
 * 3 -> non significativo
 * 2 -> porta udp impostata
 * 1 -> nome impostato
 * 0 -> id ricevuto
*/


