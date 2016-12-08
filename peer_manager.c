#include "peer_manager.h"

/******variabili e metodi privati del gestore***/
int last_pos = -1; //ultima posizione usata
char position_free = 0;
unsigned int max_peers = 50;
void alloc_peer( des_peer** p );
/***********************************************/

/**variabili pubbliche**/
des_peer** peers;
/************************/


int get_index_peer_sock( int sockt )
{
	int i;
	for( i=0; i<=last_pos; i++)
	{
		if( (peers[i] != NULL) && (peers[i]->conn.socket == sockt) )
			return i;
	}
	return -1;
}

int get_index_peer_name( char* name )
{
	int i;
	for( i=0; i<=last_pos; i++)
	{
		if( (peers[i] != NULL) && (strcmp(peers[i]->name,name)==0) )
			return i;
	}
	return -1;
}

int add_peer( int n_peers_connected )
{
	int i;

	if( n_peers_connected == max_peers )
		return -1;

	if( n_peers_connected == 0 )
	{
		printf("--aggiungo peer in pos. 0\n");
		/*peers[0] = (des_peer*)malloc(sizeof(des_peer));
		peers[0]->state = UNSET; */
		alloc_peer(&peers[0]);
		last_pos = 0;
		//alloc_peer(&peers[0]);
		return 0;
	}

	/*per evitare di scorrere inutilmente si puo' settare
	  una variabile quando si rimuove un descrittore di peer.

	  se quella variabile e' falsa si aggiunge alla fine.
	  se quella variabile e' vera si fa il ciclo.

	  quella variabile viene settata falsa se il ciclo viene
	  eseguito e non si trova un campo vuoto.*/

	if( position_free ) {
		for(i=0; i<=last_pos; i++)
		{
			if( peers[i] == NULL )
			{
				printf("--aggiungo peer in pos. %d\n",i);
				/*peers[i] = (des_peer*)malloc(sizeof(des_peer));
				peers[i]->state = UNSET;*/ 
				alloc_peer(&peers[i]);
				return i;
			}
		}
		position_free = 0;
	}
	last_pos++;

	printf("--aggiungo peer in pos. %d\n",last_pos);
	/*peers[last_pos] = (des_peer*)malloc(sizeof(des_peer));
	peers[last_pos]->state = UNSET; */
	alloc_peer(&peers[last_pos]);
	return last_pos;
}

void alloc_peer( des_peer** p )
{
	*p = (des_peer*)malloc(sizeof(des_peer));
	(*p)->state = UNSET;
}

int remove_peer_having_sock( int sockt )
{
	int index = get_index_peer_sock(sockt);
	int res = -1;
	//int i, last;

	if( index != -1 ) 
	{
		printf("rimuovo il peer di indice %d\n",index);
		free(peers[index]);
		peers[index] = NULL;

		position_free = 1; //si e' liberato un posto

		/*se rimuovo l'ultimo elemento aggiorno la variabile*/
		if( index == last_pos )
			last_pos--;

		return index;
	}

	return res;
}

int get_max_peers()
{
	return max_peers;
}

int get_peers_name( char** list, int n_peers )
{
	int i;
//	char pre[] = {"LIST OF PEERS: \n"};
	char s_libero[] = {"(libero)\n"};
	char s_occupato[] = {"(occupato)\n"};

	//int n_pre = strlen(pre);
	int n_state = strlen(s_occupato);

	if( *list != NULL )
		return -1;

	/*Ogni stringa occupa: NAME_LEN byte (NAME_LEN-1) per il nome + stato + '\n'.
	 *In totale = len_pre + spazio_singolo * n_peers + '\0'; */
	*list = (char*)malloc( (sizeof(char)*NAME_LEN+n_state)*n_peers+1 );
//	strcpy(*list, pre) ;

	for( i=0; i<n_peers && i<=last_pos; i++) {
		/*Dato che l'array puo' contenere buchi
		 *controllo che l'elemento sia valido.
		 *Inserisco solo i peer che sono registrati. */
		if( peers[i] != NULL ) {
			if( peers[i]->state == PEER_FREE ){
				strcat(*list,peers[i]->name);
				strcat(*list,s_libero);
			} else if( peers[i]->state == PEER_PLAYING ) {
				strcat(*list,peers[i]->name);
				strcat(*list,s_occupato);
			}
		}
	}

	return strlen(*list)+1;
}