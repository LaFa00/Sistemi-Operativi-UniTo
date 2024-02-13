
#ifndef MASTER
#define MASTER

#include "model.h"
#include <limits.h>

#define TEST_ERROR    if (errno) {fprintf(stderr, \
					   "%s:%d: PID=%5d: Error %d (%s)\n",\
					   __FILE__,\
					   __LINE__,\
					   getpid(),\
					   errno,\
					   strerror(errno));exit(-1);}
                       
#define CONFIG "./config/config.txt"





typedef struct {
    Conf* config;
}Shared_Conf;

typedef struct {
    double *map;
	Coordinates* port;
}Shared_Map;




sigset_t all;

/*Struttura configurazione*/
Conf *config;

/* Array per salvare temporaneamente le
coordinate delle celle source */ 

/*Struct per salvare le coordinate della mappa*/
Coordinates* coord_port; 

/*Struct per salvare coordinate nave*/
Coordinates* coord_ship;

/*Array pid delle navi*/
pid_t *pid_ship;

/*Array pid dei porti*/
pid_t *pid_port;

sigset_t mask;/* Contatore di giorni della simulazione */ 


/*Puntatore alla matrice che rappresenta la mappa*/
double **map;

/**/
int giorni = 0;

/*Array inizializzazione merci*/



/* ID semaforo per sincronizzazione.
[0] --> sync iniziale
[1] --> sync per l'accesso alla msg queue
[2] --> sync per l'accesso alla shared memory delle stats dei taxi */
int sem_id;




int sem_days;

/*Semaforo accesso memoria condivisa porto*/
int sem_port;

/* ID shared memory per per passare i valori della mappa*/  
int shm_master;

/*ID shared map*/
int shm_map;

/*ID shared  porti*/
int port_id;

int ship_id;

int merci_id_nave;
int merci_id_porto;

int msq_id;

int report_id;

Report *report;

/*ID SHARED coordinate porti*/
int port_coord_id;

/*ID shared coordinate navi*/
int ship_coord_id;


/*Puntatore ad array che memorizza tutti i porti */
Porto* porti;

Nave* navi;

Merce* merci_nave;
Merce* merci_porto;


int giorni_simulazione_id;
int *giorni_simulazione;
int processi_morti;




/* ##### PROTOTIPI FUNZIONI ##### */

/*Setting configurazione della simulazione */
bool set_conf();

/*Check della configurazione della simulazione*/
bool check_conf();

/*Generazione dei porti nella mappa*/
void gen_port();

/*Generazione delle navi nella mappa*/
void gen_ship();

/*Genera le merci*/
void gen_merci();

/*Generazione delle celle dei porti*/
void gen_port_cells();

/*Generazione celle navi*/
void gen_ship_cells();

/*Generazione della mappa*/
void gen_map();

/*Generatore numeri double random*/
double generate_random_double(double min, double max);


/*Funzione inizzializazione della simulazione*/
void init();

/*Funzione inizializzazione semafori*/
void gen_semaphores();

/*Funzione che inizializza memorie condivise e semafori*/
void create_shm_sem();

void init_shared_memories();

/*Funzione creazione memoria condivisa per la configurazione*/
 void init_shm_config();

/*Funzione memoria condivisa per la mappa*/
void init_shm_map();

/*Funzione che inizializza semaforo memoria condivisa porto*/
void init_semaphores();

void init_message_queue();

void handle_signals(int sig);

void daily_report();

void stampa_navi();

bool check_merci_nave();

bool check_merci_porto();

void final_report(); 


int is_coordinate_duplicate(Coordinates *port_coordinates, int count, int x, int y);

/*Crea memoria condivisa per passare i lotti*/
 /*void init_shm_good_master_to_port();*/

/*Stampa della mappa*/
void print_map();


void start_signals();
void read_shm_map();
void kill_all_process();
void deallocate_IPCs();
void simulation();
void clean();
void clean_mat(double** map);


#endif