#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define NUM_TRACKS 1

/* prototipo de funciones */
void *trackboss_function(void*);
void *radar_function(void*);
void *control_function(void*);
void *control_function2(void*);

/* declaracion de los mutex */
pthread_mutex_t mut_id;				/* mutex para controlar la asignacion ordenada de los id a medida que se crean los aviones */
pthread_mutex_t mut_buffer;			/* mutex para controlar el acceso al recurso buffer o cola */

pthread_cond_t lleno;				/* variable condicion para el caso en el que la cola se llene */
pthread_cond_t vacio;				/* variable condicion para el caso en el que la cola este vacia */
pthread_cond_t ultimo;				/* variable condicion para asegurar que el ultimo avion de la jornada sea encolado de ultimo */

int n_elements = 0;					/* numero de aviones en la cola */

int n_planes_takeoff;				/* numero de aviones que deben despegar */
int time_takeoff;					/* tiempo necesario para que los aviones despeguen */
int n_planes_land;					/* numero de aviones que deben aterrizar */
int time_landing;					/* tiempo necesario para que los aviones aterricen */
int size;							/* tamaño de la cola */
int exe_mode = 0;					/* modo de ejecucion (0 por defecto) */

int id_counter = 0;					/* recurso compartido controlado por el mutex "mutex_id" para la asignacion de los id */

int planes_processed = 0;			/* numero total de aviones procesados por la o las torres de control */
int planes_processed_control_1 = 0;	/* numero total de aviones procesados por la torre de control 1 */
int planes_processed_control_2 = 0;	/* numero total de aviones procesados por la torre de control 2 */
int planes_takenoff = 0;			/* numero total de aviones que despegaron */
int planes_landed = 0;				/* numero total de aviones que aterrizaron */
int planes_queued = 0;				/* numero de aviones que han sido encolados */

void print_banner()
{
    printf("\n*****************************************\n");
    printf("Welcome to ARCPORT - The ARCOS AIRPORT.\n");
    printf("*****************************************\n\n");
}


int main(int argc, char ** argv) {

	/* si el programa se ejecuta sin argumentos se toman los valores por defecto */
	if (argc == 1) {

		n_planes_takeoff = 4;
		time_takeoff = 2;
		n_planes_land = 3;
		time_landing = 3;
		size = 6;
	}

	/* si el programa se ejecuta con argumentos estos se asignan a las variables correspondientes previamente declaradas */
	else if (argc == 6 || argc == 7) {

		n_planes_takeoff = atoi(argv[1]);
		time_takeoff = atoi(argv[2]);
		n_planes_land = atoi(argv[3]);
		time_landing = atoi(argv[4]);
		size = atoi(argv[5]);
		if (argc == 7) exe_mode = atoi(argv[6]);
	
		/* comprueba que ninguno de los argumentos sea menor a 0 */
		if (n_planes_takeoff < 0 || n_planes_land < 0 || time_takeoff < 0 || time_landing < 0) {
			printf("ERROR ningún argumento puede ser negativo\n");
			exit(-1);
		}

		/* comprueba que el tamaño de la cola sea mayor a 0 */
		if (size <= 0) {
			printf("ERROR el tamaño de la cola debe ser mayor a 0\n");
			exit(-1);
		}

		/* comprueba que el argumento exe_mode sea 1 */
		if (argc == 7 && exe_mode != 1) {
			printf("ERROR el argumento \"exe_mode\" debe tomar el valor de 1 en caso de ejecutar en modo de dos pistas\n");
			exit(-1);
		}
	}

	/* si el numero de argumentos es incorrecto se imprimen los mensajes correspondientes */
	else {

		printf("usage 1: ./arcport\n");
		printf("usage 2: ./arcport <n_planes_takeoff> <time_to_takeoff> <n_planes_to_arrive> <time_to_arrive> <size_of_buffer>\n");
		printf("usage 3: ./arcport <n_planes_takeoff> <time_to_takeoff> <n_planes_to_arrive> <time_to_arrive> <size_of_buffer> <exe_mode>\n");
		exit(-1);
	}

    print_banner();

    /* creacion de la cola con el tamaño correspondiente */
    queue_init(size);

    /*declaracion de los threads */
    pthread_t trackboss, radar, control, control_2;

    /* creacion de los mutex */
    pthread_mutex_init(&mut_id, NULL);
    pthread_mutex_init(&mut_buffer, NULL);

    /* creacion de las variables condicion */
    pthread_cond_init(&lleno, NULL);
    pthread_cond_init(&vacio, NULL);
    pthread_cond_init(&ultimo, NULL);

    /* creacion de los threads */
    pthread_create(&trackboss, NULL, trackboss_function, NULL);
    pthread_create(&radar, NULL, radar_function, NULL);
    pthread_create(&control, NULL, control_function, NULL);
    pthread_create(&control_2, NULL, control_function2, NULL);

    /* espera a que los threads terminen su ejecucion */
    pthread_join(trackboss, NULL);
    pthread_join(radar, NULL);
    pthread_join(control, NULL);
    pthread_join(control_2, NULL);;

    /* destruccion de los recursos creados */
    pthread_mutex_destroy(&mut_id);
    pthread_mutex_destroy(&mut_buffer);
    pthread_cond_destroy(&lleno);
    pthread_cond_destroy(&vacio);
    pthread_cond_destroy(&ultimo);

    /* destruccion de la cola */
    queue_destroy();
    
    int fd_output = open("resume.air", O_TRUNC | O_WRONLY | O_CREAT, 0744);

    /* comprueba si hubo error al crear o truncar el fichero */
    if (fd_output == -1) {

		printf("ERROR al crear o truncar el fichero de entrada\n");
		exit(-1);
	}

	int string_lines;

	if (exe_mode) string_lines = 5;
	else string_lines = 3;

	char * strings[string_lines];		/* cadenas de caracteres auxiliares para escribir en el fichero */
	
	int i;
	for (i = 0; i < string_lines; i++) {
		strings[i] = NULL;
	}

	char * lines[string_lines + 1];		/* cadenas de caracteres a escribir en el fichero */
	
	strings[0] = "\tTotal number of planes processed: \0";
	char string0[50];
	strcpy(string0, strings[0]);
	char str0[12];
	sprintf(str0, "%d", planes_processed);
	lines[0] = strcat(string0, str0);

	strings[1] = "\n\tNumber of planes landed: \0";
	char string1[50];
	strcpy(string1, strings[1]);
	char str1[12];
	sprintf(str1, "%d", planes_landed);
	lines[1] = strcat(string1, str1);

	strings[2] = "\n\tNumber of planes taken off: \0";
	char string2[50];
	strcpy(string2, strings[2]);
	char str2[12];
	sprintf(str2, "%d", planes_takenoff);
	lines[2] = strcat(string2, str2);

	if (exe_mode) {

		strings[3] = "\n\tNumber of planes processed in track 1: \0";
		char string3[50];
		strcpy(string3, strings[3]);
		char str3[12];
		sprintf(str3, "%d", planes_processed_control_1);
		lines[3] = strcat(string3, str3);

		strings[4] = "\n\tNumber of planes processed in track 2: \0";
		char string4[50];
		strcpy(string4, strings[4]);
		char str4[12];
		sprintf(str4, "%d", planes_processed_control_2);
		lines[4] = strcat(string4, str4);
	}

	lines[string_lines] = "\n";

	int j, writtenBytes;

	for (j = 0; j < string_lines + 1; j++) {

		writtenBytes = write(fd_output, lines[j], (int)strlen(lines[j]));

		/* comprueba si hubo error al escribir en el fichero */
		if (writtenBytes == -1) {

				printf("ERROR al escribir en el fichero \"resume.air\"\n");
				exit(-1);
		}
	}

	int close_fd_output = close(fd_output);

	/* Comprueba si hubo error al cerrar el fichero de salida */
	if (close_fd_output == -1) {

		printf("ERROR al cerrar el fichero de salida\n");
		exit(-1);
	}

    exit(0);
}

/* TRACKBOSS */
void *trackboss_function(void* p) {

	int i;
	struct plane plane_checked;

	for(i = 0; i < n_planes_takeoff; i++) {

		pthread_mutex_lock(&mut_id);		/* solicita el mutex "mut_id" */
		plane_checked.id_number = id_counter;
		plane_checked.time_action = time_takeoff;
		plane_checked.action = 0;			/* 0 para despegar */
		/* si es el ultimo avion se le asigna el flag correspondiente */
		if (id_counter == n_planes_takeoff + n_planes_land - 1) plane_checked.last_flight = 1;
		else plane_checked.last_flight = 0;
		printf("[TRACKBOSS] Plane with id %d checked\n", id_counter);
		id_counter++;
		pthread_mutex_unlock(&mut_id);		/* libera el mutex "mut_id" */

		pthread_mutex_lock(&mut_buffer);	/* solicita el mutex "mut_buffer" */
		/* mientras la cola este llena se suspende en la variable condicion "lleno" */
		while (n_elements == size) pthread_cond_wait(&lleno, &mut_buffer);
		/* si es el ultimo avion de la jornada y aun quedan aviones con id inferior por encolar, se suspende en la variable condicion "ultimo" */
		if (plane_checked.last_flight) {
			while (planes_queued != n_planes_takeoff + n_planes_land - 1) pthread_cond_wait(&ultimo, &mut_buffer);
		}
		queue_put(&plane_checked);			/* encola el avion creado */
		planes_queued++;
		printf("[TRACKBOSS] Plane with id %d ready to take off\n", plane_checked.id_number);
		n_elements++;
		/* se despierta a los threads suspendidos en la variable condicion "vacio" */
		if (n_elements >= 1) pthread_cond_signal(&vacio);
		/* se despierta a los threads suspendidos en la variable condicion "ultimo" */
		if (planes_queued == n_planes_takeoff + n_planes_land - 1) pthread_cond_signal(&ultimo);
		pthread_mutex_unlock(&mut_buffer);	/* libera el mutex "mut_buffer" */
	}
	pthread_exit(0);
}

/* RADAR */
void *radar_function(void* p) {

	int i;
	struct plane plane_detected;

	for(i = 0; i < n_planes_land; i++) {

		pthread_mutex_lock(&mut_id);		/* solicita el mutex "mut_id" */
		plane_detected.id_number = id_counter;
		plane_detected.time_action = time_takeoff;
		plane_detected.action = 1;			/* 1 para aterrizar */
		/* si es el ultimo avion se le asigna el flag correspondiente */
		if (id_counter == n_planes_takeoff + n_planes_land - 1) plane_detected.last_flight = 1;
		else plane_detected.last_flight = 0;
		printf("[RADAR] Plane with id %d detected\n", id_counter);
		id_counter++;
		pthread_mutex_unlock(&mut_id);		/* libera el mutex "mut_id" */

		pthread_mutex_lock(&mut_buffer);	/* solicita el mutex "mut_buffer" */
		/* mientras la cola este llena se suspende en la variable condicion "lleno" */
		while (n_elements == size) pthread_cond_wait(&lleno, &mut_buffer);
		/* si es el ultimo avion de la jornada y aun quedan aviones con id inferior por encolar, se suspende en la variable condicion "ultimo" */
		if (plane_detected.last_flight) {
			while (planes_queued != n_planes_takeoff + n_planes_land - 1) pthread_cond_wait(&ultimo, &mut_buffer);
		}
		queue_put(&plane_detected);			/* encola el avion creado */
		planes_queued++;
		n_elements++;
		printf("[RADAR] Plane with id %d ready to land\n", plane_detected.id_number);
		/* se despierta a los threads suspendidos en la variable condicion "vacio" */
		if (n_elements >= 1) pthread_cond_signal(&vacio);
		/* se despierta a los threads suspendidos en la variable condicion "ultimo" */
		if (planes_queued == n_planes_takeoff + n_planes_land - 1) pthread_cond_signal(&ultimo);
		pthread_mutex_unlock(&mut_buffer);	/* libera el mutex "mut_buffer" */
	}
	pthread_exit(0);
}

/* CONTROL_1 */
void *control_function(void* p) {

	struct plane * plane_ready;

	while (planes_processed != n_planes_takeoff + n_planes_land) {

		pthread_mutex_lock(&mut_buffer);		/* solicita el mutex "mut_buffer" */
		/* mientras la cola se encuentre vacia se suspende en la variable condicion "vacio" */
		while (n_elements == 0 && planes_processed != n_planes_takeoff + n_planes_land) {
			if (exe_mode) printf("[CONTROL_1] Waiting for planes in empty queue\n");
			else printf("[CONTROL] Waiting for planes in empty queue\n");
			pthread_cond_wait(&vacio, &mut_buffer);
		}
		if (planes_processed == n_planes_takeoff + n_planes_land) pthread_exit(0);
		plane_ready = queue_get();				/* desencola el avion correspondiente */
		n_elements--;
		planes_processed++;						/* suma uno al numero de aviones procesados */
		planes_processed_control_1++;			/* suma uno al numero de aviones procesados por la torre de control 1 */
		/* si el avion va a aterrizar imprime el mensaje correspondiente */
		if (plane_ready->action) {
			if (exe_mode) printf("[CONTROL_1] Track is free for plane with id %d\n", plane_ready->id_number);
			else printf("[CONTROL] Track is free for plane with id %d\n", plane_ready->id_number);
			planes_landed++;					/* suma uno al numero de aviones aterrizados */
		}
		/* si el avion va a despegar imprime el mensaje correspondiente */
		else {
			if (exe_mode) printf("[CONTROL_1] Putting plane with id %d in track\n", plane_ready->id_number);
			else printf("[CONTROL] Putting plane with id %d in track\n", plane_ready->id_number);
			planes_takenoff++;					/* suma uno al numero de aviones despegados */
		}
		/* si es el ultimo avion imprime el mensaje correspondiente */
		if (plane_ready->last_flight) {
			if (exe_mode) printf("[CONTROL_1] After plane with id %d the airport will be closed\n", plane_ready->id_number);
			else printf("[CONTROL] After plane with id %d the airport will be closed\n", plane_ready->id_number);
		}
		/* si el avion va a despegar imprime el mensaje correspondiente */
		if (!plane_ready->action) {
			if (exe_mode) printf("[CONTROL_1] Plane %d took off after %d seconds\n", plane_ready->id_number, plane_ready->time_action);
			else printf("[CONTROL] Plane %d took off after %d seconds\n", plane_ready->id_number, plane_ready->time_action);
		}
		/* si el avion va a aterriar imprime el mensaje correspondiente */
		else {
			if (exe_mode) printf("[CONTROL_1] Plane %d landed in %d seconds\n", plane_ready->id_number, plane_ready->time_action);
			else printf("[CONTROL] Plane %d landed in %d seconds\n", plane_ready->id_number, plane_ready->time_action);
		}
		/* el thread suspende su ejecucion de acuerdo al tiempo de despegue o aterrizaje del avion */
		sleep(plane_ready->time_action);
		/* si se procesaron todos los aviones imprime el mensaje de cierre de aeropuerto */
		if (planes_processed == n_planes_takeoff + n_planes_land) {
			printf("Airport Closed!\n");
			pthread_cond_signal(&vacio);	/* si se ejecuta en modo "dos torres de control", al final habra una torre suspendida en la variable condicion "vacio" */
		}
		/* despierta a los threads suspendidos en la variable condicion "lleno" */
		if (n_elements < size) pthread_cond_signal(&lleno);
		/* si solo queda un avion por encolar se despierta a los threads suspendidos en la variable condicion "ultimo" */
		if (planes_queued == n_planes_takeoff + n_planes_land - 1) pthread_cond_signal(&ultimo);
		pthread_mutex_unlock(&mut_buffer);		/* libera el mutex "mut_buffer" */
	}
	pthread_exit(0);
}

/* CONTROL_2 */
void *control_function2(void* p) {

	if (exe_mode) {

		struct plane * plane_ready;

		while (planes_processed != n_planes_takeoff + n_planes_land) {

			pthread_mutex_lock(&mut_buffer);		/* solicita el mutex "mut_buffer" */
			/* mientras la cola se encuentre vacia se suspende en la variable condicion "vacio" */
			while (n_elements == 0 && planes_processed != n_planes_takeoff + n_planes_land) {
				printf("[CONTROL_2] Waiting for planes in empty queue\n");
				pthread_cond_wait(&vacio, &mut_buffer);
			}
			if (planes_processed == n_planes_takeoff + n_planes_land) pthread_exit(0);
			plane_ready = queue_get();				/* desencola el avion correspondiente */
			n_elements--;
			planes_processed++;						/* suma uno al numero de aviones procesados */
			planes_processed_control_2++;			/* suma uno al numero de aviones procesados por la torre de control 2 */
			/* si el avion va a aterrizar imprime el mensaje correspondiente */
			if (plane_ready->action) {
				printf("[CONTROL_2] Track is free for plane with id %d\n", plane_ready->id_number);
				planes_landed++;					/* suma uno al numero de aviones aterrizados */
			}
			/* si el avion va a despegar imprime el mensaje correspondiente */
			else {
				printf("[CONTROL_2] Putting plane with id %d in track\n", plane_ready->id_number);
				planes_takenoff++;					/* suma uno al numero de aviones despegados */
			}
			/* si es el ultimo avion imprime el mensaje correspondiente */
			if (plane_ready->last_flight) printf("[CONTROL_2] After plane with id %d the airport will be closed\n", plane_ready->id_number);
			/* si el avion va a despegar imprime el mensaje correspondiente */
			if (!plane_ready->action) printf("[CONTROL_2] Plane %d took off after %d seconds\n", plane_ready->id_number, plane_ready->time_action);
			/* si el avion va a aterriar imprime el mensaje correspondiente */
			else printf("[CONTROL_2] Plane %d landed in %d seconds\n", plane_ready->id_number, plane_ready->time_action);
			/* el thread suspende su ejecucion de acuerdo al tiempo de despegue o aterrizaje del avion */
			sleep(plane_ready->time_action);
			/* si se procesaron todos los aviones imprime el mensaje de cierre de aeropuerto */
			if (planes_processed == n_planes_takeoff + n_planes_land) {
				printf("Airport Closed!\n");
				pthread_cond_signal(&vacio);	/* si se ejecuta en modo "dos torres de control", al final habra una torre suspendida en la variable condicion "vacio" */
			}
			/* despierta a los threads suspendidos en la variable condicion "lleno" */
			if (n_elements < size) pthread_cond_signal(&lleno);
			/* si solo queda un avion por encolar se despierta a los threads suspendidos en la variable condicion "ultimo" */
			if (planes_queued == n_planes_takeoff + n_planes_land - 1) pthread_cond_signal(&ultimo);
			pthread_mutex_unlock(&mut_buffer);		/* libera el mutex "mut_buffer" */
		}
	}
	pthread_exit(0);
}