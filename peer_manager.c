#include "peer_manager.h"

/******variabili e metodi privati del gestore**********/

/*vettore di puntatori a descrittori di peer*/
static des_peer** peers_;

/*ultima posizione usata*/
static int last_pos_;

/*vi è una posizione libera(in mezzo)*/
static char position_free_;

/*quanti elementi al massimo può contenere peers*/
static unsigned int max_peers_;

/*numero di peers attualmente inseriti*/
static unsigned int n_peers_; 

/*Alloca un des_peer e lo inizializza ad UNSET.
 *Si aspetta come parametro un indirizzo di puntatore
 *a des_peer. Dopo la chiamata il puntatore conterrà
 *l'indirizzo del nuovo des_peer allocato
 */
static void alloc_peer( des_peer** p );

/*****************************************************/

int init_peer_manager()
{
	max_peers_ = 10;
	n_peers_ = 0;
	position_free_ = 0;
	last_pos_ = -1;
      
      /*Alloco il vettore che contiene max_peers puntatori a peers*/
	size_t size_peers = sizeof(des_peer*) * max_peers_;
	peers_ = (des_peer**)malloc(size_peers);
	if( peers_ == NULL )
		return 0;
	/*azzero tutti i puntatori*/
	memset( peers_, 0, size_peers ); 

	return 1;
}

int more_peers()
{
	//int i;
	unsigned int old_max = max_peers_;
	des_peer** new_v = NULL;

	max_peers_ *= 2;
	new_v = (des_peer**)malloc(sizeof(des_peer*)*max_peers_);
	if( new_v == NULL )
		return -1;

	/*azzero l'array di puntatori e poi ci copio
	  i vecchi valori*/
	memset(new_v,0,sizeof(des_peer*)*max_peers_); 
	memcpy(new_v,peers_,sizeof(des_peer*)*old_max);

	/*for( i=0; i<old_max; i++ )
		new_v[i] = peers[i]; */
	free(peers_);
	peers_ = new_v;
	return max_peers_;
}

int get_index_peer_sock( int sockt )
{
	int i;
	for( i=0; i<=last_pos_; i++)
	{
		if( (peers_[i] != NULL) && (peers_[i]->conn.socket == sockt) )
			return i;
	}
	return -1;
}

int get_index_peer_name( char* name )
{
	int i;
	if( n_peers_ == 0 )
		return -1;

	for( i=0; i<=last_pos_; i++)
	{
		#ifdef DEBUG
		printf("peers[%d]=%p\n",i,peers_[i]);
		#endif

		if( (peers_[i] != NULL) && (strcmp(peers_[i]->name,name)==0) )
			return i;
	}
	return -1;
}

int add_peer( )
{
	int i;

	if( n_peers_ == max_peers_ ) 
	{
		i = more_peers();
		if( i==-1 ) {
			printf("Errore allocazione nuovo vettore\n");
			return -1;
		}
	}

	if( n_peers_ == 0 )
	{
		#ifdef DEBUG
		printf("--aggiungo peer in pos. 0\n");
		#endif
		alloc_peer(&peers_[0]);
		last_pos_ = 0; 
		return 0;
	}

	/*per evitare di scorrere inutilmente si puo' settare
	  una variabile quando si rimuove un descrittore di peer.

	  se quella variabile e' falsa si aggiunge alla fine.
	  se quella variabile e' vera si fa il ciclo.

	  quella variabile viene settata falsa se il ciclo viene
	  eseguito e non si trova un campo vuoto.*/

	if( position_free_ ) 
	{
		for(i=0; i<=last_pos_; i++)
		{
			if( peers_[i] == NULL )
			{
				#ifdef DEBUG
				printf("--aggiungo peer in pos. %d\n",i);
				#endif
				alloc_peer(&peers_[i]);
				return i;
			}
		}
		position_free_ = 0;
	}
	last_pos_++;

	#ifdef DEBUG
	printf("--aggiungo peer in pos. %d\n",last_pos_);
	#endif

	alloc_peer(&peers_[last_pos_]);
	return last_pos_;
}

static
void alloc_peer( des_peer** p )
{
	*p = (des_peer*)malloc(sizeof(des_peer));
	memset(*p,0,sizeof(des_peer)); 
	(*p)->state = UNSET;
	n_peers_++;
}

/*int remove_peer_having_sock( int sockt )
{
	int index = get_index_peer_sock(sockt);
      return remove_peer(index);
}*/

int get_n_peers()
{
	return n_peers_;
}

int get_peers_name( char** list )
{
	int i, ins;
	char s_libero[] = {"(libero)\n"};
	char s_occupato[] = {"(occupato)\n"};

	//int n_pre = strlen(pre);
	int n_state = strlen(s_occupato);

	if( *list != NULL )
		return -1;

	/*Ogni stringa occupa: NAME_LEN byte (NAME_LEN-1) per il nome + stato + '\n'.
	 *In totale = len_pre + spazio_singolo * n_peers + '\0'; */
	*list = (char*)malloc(NAME_LEN + n_state*n_peers_ + 1 );
//	strcpy(*list, pre) ;

	for( i=0, ins=0; ins<n_peers_; i++ ) {
		/*Dato che l'array puo' contenere buchi
		 *controllo che l'elemento sia valido.
		 *Inserisco solo i peer che sono registrati. */
		if( peers_[i] != NULL ) {
			if( peers_[i]->state == PEER_FREE ){
				strcat(*list,peers_[i]->name);
				strcat(*list,s_libero);
			} else if( peers_[i]->state == PEER_PLAYING ) {
				strcat(*list,peers_[i]->name);
				strcat(*list,s_occupato);
			}
			ins++;
		}
	}

	return strlen(*list)+1;
}

int is_valid_id( int id )
{
	if( id >= max_peers_ )
		return 0;
	if( peers_[id] == NULL )
		return 0;
	else
		return 1;
}

des_peer* get_peer( int index )
{
	if( index >= max_peers_ )
		return NULL;
	return peers_[index];
}

int remove_peer( int index )
{
	if( !is_valid_id(index) )
		return 0;
	
	#ifdef DEBUG
	printf("rimuovo peer da indice %d\n",index);
	#endif

	/*leggi nota alla fine*/
/*	memset(peers[index],0,sizeof(des_peer)); */
	free(peers_[index]);
	peers_[index] = NULL;
	n_peers_--;

	/*se rimuovo l'ultimo elemento aggiorno la variabile*/
	if( index == last_pos_ )
		last_pos_--;
	else
		position_free_ = 1; /*si e' liberato un posto in mezzo*/

	#ifdef DEBUG
	printf("last_pos:%d position_free:%d n_peers:%d\n",last_pos_,position_free_,n_peers_);
	#endif

	return 1;
}


/* Nota
Bug riscontrato quando un client si disconnette e poi si ricollega subito 
ottenendo lo stesso id. Se il client inserisce lo stesso nome che aveva
prima riceve dal server il messaggio NAME_REFUSED. E' un messaggio che non
dovrebbe ricevere perchè non esiste alcun peer con quel nome. Ciò è dovuto 
al fatto che la malloc rialloca la memoria per il des_peer nelle stesse 
locazioni e pertato si provoca il conflitto. Per evitare ogni problema 
prima di fare free(peer[index]) azzero l'area di memoria destinata al 
descrittore di peer.*/
