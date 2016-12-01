#include "game.h"

int main(int argc, char** argv)
{
	int x = 0;
	battle_game bg;
	coordinate co;

	init_local_game(&bg);

	while(x>=0){
		printf("Inserisci le coordinate: ");
		scanf(" %c %c",&co.x,&co.y);
		#ifdef DEBUG
		printf("%c %c\n",co.x,co.y);
		#endif
		x = set_ship(&co,&bg);
		#ifdef DEBUG
		printf("debug=%d\n",x);
		#endif
		show_grind(&bg);
	}
	printf("--NAVI POSIZIONATE\n");

	while(1){
		printf("Inserisci le coordinate: ");
		scanf(" %c %c",&co.x,&co.y);
		x = shot_ship(&co, &bg);
		#ifdef DEBUG
		printf("sparo=%d navi_colpite=%d\n",x,n_ship_hit(&bg));
		#endif
		show_grind(&bg);
	}

	return 0;
}