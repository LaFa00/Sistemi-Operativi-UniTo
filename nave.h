#include "model.h"
#include "master.h"
#ifndef NAVE
#define NAVE
#define OFFERT 1
#define REQUEST 0

Conf *config_arr; /*Array contenente configurazione*/

double **shared_map; /*Mappa del mondo condivisa dal master*/

Nave* nave; /*Struct nave*/

Porto* porti; 

Merce* merci;



sigset_t all;


/*Coordinate porto*/
double r_ship;
double c_ship;


/*Indice nave processo che esegue*/
int indice_nave;

int sem_id_nave;

int sem_id_banchine;

int msqid_nave; /*ID CODA DI MESSAGGI*/

int num_banchine;

int merci_id;

int check_port = 0;

int statoNave;

int comunicato;

int pidPortoDestinazione;


void init_shared_memories(char *argv[]);
void init_semaphores(int sem_id);
void initialize_config(int shared_id);
void initialize_map(int shared_id);
void initialize_ship(char*argv[]);
void handle_signals(int sig);
int calculate_total_cargo_weight(Merce* merci);
int genera_quantita_richiesta();
void start_signals();
int find_banchina();
int min();
void clean();



void find_port();
void go_to_port();
void comunicazione();
double calcola_distanza(Coordinates nave, Coordinates porto);


#endif


