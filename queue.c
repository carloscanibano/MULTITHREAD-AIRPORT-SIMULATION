#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include <string.h>

struct plane * planes = NULL;		/* buffer circular (cola) */
int planes_size;					/* tamaño del buffer circular (numero de aviones que caben como maximo) */
int counter = 0;					/* numero actual de aviones en el buffer */
int pos = 0;						/* posicion del buffer en la que se debe introducir el siguiente avion */
struct plane result;				/* estructura para devolver los aviones al desencolar del buffer */

/*To create a queue*/
int queue_init(int size){

	/* reserva espacio en memoria del tamaño correspondiente de acuerdo al argumento recibido */
	planes = (struct plane *)calloc(size, sizeof(struct plane));
	planes_size = size;
    return 0;
}

/* To Enqueue an element*/
int queue_put(struct plane* x) {

	/* se encola el avion en la posicion correspondiente. El caso de que este lleno o no el buffer se controla desde arcport.c */
	planes[pos] = *x;
	/* el maximo valor que toma "pos" sera el tamaño del buffer */
	if(pos < planes_size) {
		pos++;
	}
	/* el maximo valor que toma "counter" sera el tamaño del buffer */
	if (counter < planes_size) {
		counter++;
	}
	printf("[QUEUE] Storing plane with id %d\n", x->id_number);

    return 0;
}


/* To Dequeue an element.*/
struct plane* queue_get(void) {

	result = planes[0];
	int i;

	for(i = 0; i < pos; i++) {

		if (i < pos - 1) planes[i] = planes[i + 1];
	}
	pos--;
	counter--;
	printf("[QUEUE] Getting plane with id %d\n", result.id_number);
	return &result;
}


/*To check queue state*/
int queue_empty(void){

	if (counter == 0) return 1;		/* devuelve 1 si esta vacia */
    return 0;						/* devuelve 1 si no esta vacia */
}

/*To check queue state*/
int queue_full(void){

	if (counter == planes_size) return 1;	/* devuelve 1 si esta llena */
    return 0;								/* devuelve 0 si no esta llena */
}

/*To destroy the queue and free the resources*/
int queue_destroy(void){

	free(planes);		/* libera la memoria reservada */
    return 0;
}
