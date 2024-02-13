#include "model.h"
#include "master.h"

#ifndef PORTO
#define PORTO

#define TEST_ERROR    if (errno) {fprintf(stderr, \
					   "%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__,\
					   __LINE__,\
					   getpid(),\
					   errno,\
					   strerror(errno));exit(-1);}
/*Stato del porto*/

/*Mappa del mondo*/
double **shared_map;

/*Coordinate porto*/
double r_port;
double c_port;

/*Numero banchine*/
int n_banchine;

int sem_id_porto;

int msqid_porto;
/*Semaforo per gestire le banchine*/
int sem_id_banchine;

int indice_porto;




/*Struct per il porto*/
Porto porto; /*Posso considerarlo come array*/


/*Struct per mantenere i dati di configurazione*/
Conf *config_arr;







/* ID shared memory per per passare i valori della 
mappa dal master ai porti */ 
int shm_master = 0;




/*PROTOTIPI FUNZIONI*/
void generate_banchine(int num_banchine,int indice_porto);

/*Inizializza strutture dati*/
void init_semaphores(int sem_id);
void init_shared_memories(char *argv[]);
void initialize_port(char *argv[]);
void initialize_config(int sharedId);
void initialize_map(int shared_id);
void handle_signals(int sig);
void start_signals();
void find_scambi();
void comunicazione_nave(int num_sem_banchina);
void clean();


/*Crea memoria condivisa*/
int generateSharedMemory(int size);

#endif



