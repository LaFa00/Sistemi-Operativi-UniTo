#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h> 
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include "master.h"
#include "model.h"
#include "nave.h"
#include "semaforo.h"

void stampa();
void stampa_merci();
void stampa_navi();
void stampa_porti();
Porto* porto_vicino;
int numero_nave;

void stampa_merci() {
    int i;
    for(i = 0; i < config_arr->SO_MERCI; i++) {
        printf("ID NAVE: %d, MERCE ID: %d\n", getpid(), nave->merci[i].id);
    }
}

void start_signals(){
    /* Struct per l'handling dei segnali e mask */
    struct sigaction sa;


    /* #### SETTING MASCHERA ### */ 
    sigemptyset(&mask); /* Svuotamento della maschera */ 

    /* Aggiunta di SIGALRM nella mask. Questo per:
    -- inviare periodicamente messaggi di richiesta */ 
    sigaddset(&mask, SIGALRM);

    /* Aggiunta di SIGQUIT nella mask. Questo per:
    -- terminare in ricezione del segnale di kill da 
       parte del master, al termine della simulazione */ 
    sigaddset(&mask, SIGQUIT);

    /* Aggiunta di SIGINT nella mask. Questo per interrompere
    la simulazione sotto comando dell'utente */ 
    sigaddset(&mask,SIGINT);

    /* Setting machera per bloccare tutti i segnali
    (durante una syscall, ad esempio) */ 
    sigfillset(&all);


    /* #### SETTING SIGACTION ### */ 
    sa.sa_flags = SA_RESTART;
    sa.sa_mask = mask;
    sa.sa_handler = handle_signals;

    /* Attivazione ascolto dei segnali con sigaction() */  
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
}

void handle_signals(int sig){
    sigprocmask(SIG_BLOCK, &mask, NULL); /* Blocco segnali nella mask */ 

    /* Switch per gestire il segnale ricevuto */ 
    switch (sig){
        /* Caso di SIGALRM */ 
        case SIGALRM:
            if(check_port == 0) {
                find_port();
               

            }
            go_to_port();
            
            alarm(2);
            break;

        /* Caso di SIGQUIT (terminazione simulazione) */ 
        case SIGQUIT:
            clean(); /* Free delle risorse */
            exit(EXIT_SUCCESS); /* Uscita dalla simulazione */ 
            break;

        /* Caso di SIGINT (terminazione simulazione per comando utente) */ 
        case SIGINT:
            raise(SIGQUIT); /* Invio segnale terminazione */
            break;
        
        /* Caso di segnale non gestito */ 
        default:
            printf("Segnale anomalo non gestito!\n"); 
            break;
    }
    
    sigprocmask(SIG_UNBLOCK, &mask, NULL); /* Sblocco dei segnali nella mask */ 
}

int main(int argc, char* argv[]) {
    int pidPortoAlto,com;
    int i,j;
    int target;
    int sem_value;
    if(argc < 12) {
        perror("Numero parametri non valido\n");
        exit(EXIT_FAILURE);
    }
    indice_nave = atoi(argv[6]);

    start_signals();
    init_shared_memories(argv);
    init_message_queue(atoi(argv[8]));
    init_semaphores(atoi(argv[7]));
    initialize_ship(argv);
    sync_process(sem_id_nave,-1);
    raise(SIGALRM);
    while(1) {
        pause();
    }
    return(-1);
    
}

void init_shared_memories(char *argv[]) {
    int port_id;
    initialize_config(atoi(argv[2])); /*ID SHARED MEMORY*/

    porti = shmat(atoi(argv[4]),NULL,0);
    if (porti == (void *) -1){
       perror("ERRORE NAVE PORTI ATTACH MEMORIA CONDIVISA");
        exit(EXIT_FAILURE);
    }

    navi = shmat(atoi(argv[5]),NULL,0);
    if (navi == (void *) -1){
        perror("ERRORE NAVI ATTACH MEMORIA CONDIVISA");
        exit(EXIT_FAILURE);
    }

    merci_id = atoi(argv[9]);

    merci_nave = shmat(merci_id,NULL,0);
    if (merci_nave == (void *) -1){
       perror("ERRORE NAVI ATTACH MEMORIA CONDIVISA MERCI");
        exit(EXIT_FAILURE);
    }
    
    giorni_simulazione_id = atoi(argv[10]);
    
    giorni_simulazione = shmat(giorni_simulazione_id,NULL,0);
    if (giorni_simulazione == (void *) -1){
       perror("ERRORE NAVI ATTACH MEMORIA CONDIVISA GIORNI");
        exit(EXIT_FAILURE);
    }

    merci_id_porto = atoi(argv[11]);

    merci_porto = shmat(merci_id_porto,NULL,0);
    if (merci_porto == (void *) -1){
       perror("ERRORE NAVI ATTACH MEMORIA CONDIVISA REPORT");
        exit(EXIT_FAILURE);
    }

}


void initialize_config(int shared_id) {
    Conf *shm_ptr;
    
    config_arr = (Conf*)malloc(sizeof(Conf));
    shm_ptr = (Conf *)shmat(shared_id,NULL,0);
    if(shm_ptr == (void *) -1) {
       perror("ERRORE ATTACCO MEMORIA CONDIVISA CONFIGURAZIONE");
       exit(EXIT_FAILURE);
    }

    memcpy(config_arr,shm_ptr,sizeof(Conf));
    shmdt((void *) shm_ptr);
}



void initialize_ship(char *argv[]) {
    int count;
    int i;
    int j;
    int index_offset;
    int offset_merce;

    r_ship = atof(argv[0]);
    c_ship = atof(argv[1]);




    lock_shm(sem_id_nave);
    navi[indice_nave].id = indice_nave;
    navi[indice_nave].pid = getpid();
    navi[indice_nave].velocita = config_arr->SO_SPEED;
    navi[indice_nave].capacity = config_arr->SO_CAPACITY;
    navi[indice_nave].posizione.x = r_ship;
    navi[indice_nave].posizione.y = c_ship;
    navi[indice_nave].merce_ricevuta = 0;
    navi[indice_nave].merce_scaricata = 0;
    navi[indice_nave].state = In_Mare_Vuota;
    offset_merce =  indice_nave * config_arr->SO_MERCI;
    navi[indice_nave].offset_to_merci = offset_merce;
    for(count = 0; count < config_arr->SO_MERCI; count++) { 
        srand(getpid() ^ clock()); 
        index_offset =  offset_merce + count;
        merci_nave[index_offset].id = count;
        merci_nave[index_offset].scadenza = 0;
        merci_nave[index_offset].merce_consegnata = 0;
        merci_nave[index_offset].quantita = 0;
        merci_nave[index_offset].offerta_domanda = 0;
        merci_nave[index_offset].stato = Nella_Nave;
    }
    
    nave = &navi[indice_nave];

    
    unlock_shm(sem_id_nave);
    

}

void find_port(){
    int i,j;
    double distanza;
    double distanza_minima;
    int count;
    distanza_minima = 1.7e308;
    lock_shm(sem_id_nave);
    for(i = 0; i < config_arr->SO_PORTI; i++) {
        distanza = calcola_distanza(nave->posizione, porti[i].posizione);
        if(distanza < distanza_minima) {
            distanza_minima = distanza;
            porto_vicino = &porti[i];
            num_banchine = porto_vicino->banchine; /*La nave conosce il numero delle banchine*/
            sem_id_banchine = porto_vicino->sem_id_banchina;
        }
    }
    unlock_shm(sem_id_nave);
}

void go_to_port() {
    struct timespec tim,tim2;
    double distanza;
    double time_travel;
    double int_part;
    double frac_part;
    
    
    if(nave->posizione.x != porto_vicino->posizione.x || nave->posizione.y != porto_vicino->posizione.y) {
        nave->state = In_Mare;
        if(nave->posizione.x < porto_vicino->posizione.x && nave->posizione.y < porto_vicino->posizione.y) {
           
            distanza = abs(porto_vicino->posizione.x - nave->posizione.x) + abs(porto_vicino->posizione.y - nave->posizione.y);
            time_travel = distanza / config_arr->SO_SPEED;
            tim.tv_sec = time_travel;
            tim.tv_nsec = (time_travel - ((int) tim.tv_sec)) * 1e9;
            nanosleep(&tim,NULL);
        } else if(nave->posizione.x > porto_vicino->posizione.x && nave->posizione.y < porto_vicino->posizione.y) {
            
            distanza = abs(nave->posizione.x - porto_vicino->posizione.x) + abs(porto_vicino->posizione.x - nave->posizione.y);
            time_travel = distanza / config_arr->SO_SPEED;
            tim.tv_sec = time_travel;
            tim.tv_nsec = (time_travel - ((int) tim.tv_sec)) * 1e9;
            nanosleep(&tim,NULL);

        } else if(nave->posizione.x < porto_vicino->posizione.x && nave->posizione.y > porto_vicino->posizione.y) {
            distanza = abs(porto_vicino->posizione.x - nave->posizione.x) + abs(nave->posizione.y - porto_vicino->posizione.y);
            time_travel = distanza / config_arr->SO_SPEED;
            
            tim.tv_sec = time_travel;
            tim.tv_nsec = (time_travel - ((int) tim.tv_sec)) * 1e9;
            nanosleep(&tim,NULL);


        } else if(nave->posizione.x > porto_vicino->posizione.x && nave->posizione.y > porto_vicino->posizione.y) {
            distanza = abs(nave->posizione.x - porto_vicino->posizione.x) + abs(nave->posizione.y - porto_vicino->posizione.x);
            time_travel = distanza / config_arr->SO_SPEED;
            tim.tv_sec = time_travel;
            tim.tv_nsec = (time_travel - ((int) tim.tv_sec)) * 1e9;
            nanosleep(&tim,NULL);
            
            


        }
        nave->posizione.x = porto_vicino->posizione.x;
        nave->posizione.y = porto_vicino->posizione.y;



    } else if(nave->posizione.x == porto_vicino->posizione.x && nave->posizione.y == porto_vicino->posizione.y) {
        nave->state = In_Porto;
        comunicazione(porto_vicino->pid);
        check_port = 0;
        nave->state = In_Mare;
    }

}

void init_semaphores(int sem_id) {
    int i;
   
    sem_id_nave = sem_id;
    if(sem_id_nave == -1) {
        perror("Errore creazione semaforo");
        exit(EXIT_FAILURE);
    }
}

void init_message_queue(int msq_id) {
    if((msqid_nave = msq_id) == -1) {
        perror("ERRORE CREAZIONE MESSAGE QUEUE NAVE");
        exit(EXIT_FAILURE);
    }
}


void comunicazione(int pidPorto) {
    int i, j;
    struct message richiesta,risposta;
    struct timespec tim;
    int merce_scambiata;
    int merce_caricata_nave;
    int merce_scaricata_nave;
    int capacita_disponibile;

    
    richiesta.mtype = porto_vicino->pid; 
    strcpy(richiesta.mText,"Richiesta di Attracco");
    sync_msg_queue(sem_id_nave,1);
    if(msgsnd(msqid_nave,&richiesta,sizeof(richiesta.mText),0) == -1) {
        perror("ERRORE INVIO RICHIESTA ATTRACCO");
    }
    sync_msg_queue(sem_id_nave,-1);

    sync_msg_queue(sem_id_nave,1);
    if(msgrcv(msqid_nave,&risposta,sizeof(risposta.mText),porto_vicino->pid,0) == -1) {
        perror("ERRORE RICEZIONE RISPOSTA ATTRACCO");
    }
   sync_msg_queue(sem_id_nave,-1);
    
    if(strcmp(risposta.mText,"ATTRACCO CONSENTITO") == 0) {
        /*Avviene carico e scarico*/
        reserveSem(porto_vicino->sem_id_banchina,0);
        porto_vicino->banchine_occupate++;
        lock_shm(sem_id_nave);
        for(i = nave->offset_to_merci, j = porto_vicino->offset_merci; i < nave->offset_to_merci + config_arr->SO_MERCI && j < config_arr->SO_MERCI + porto_vicino->offset_merci; i++, j++) {
           /*Carica il porto, domanda la nave*/
           if(merci_porto[j].scadenza >= *giorni_simulazione && merci_porto[j].offerta_domanda == 1 && merci_nave[i].offerta_domanda == 0 && merci_porto[j].quantita > 0) {
                nave->state = In_Porto_Per_Carico;
               
                capacita_disponibile = config_arr->SO_CAPACITY - calculate_total_cargo_weight(merci_nave);
                merce_caricata_nave = min(merci_porto[j].quantita, capacita_disponibile);
                if(merce_caricata_nave > 0 && capacita_disponibile > 0) {
                    porto_vicino->merce_spedita = porto_vicino->merce_spedita + merce_caricata_nave;
                    merci_nave[i].quantita = merci_nave[i].quantita + merce_caricata_nave;
                    nave->merce_ricevuta += merce_caricata_nave;
                    merci_nave[i].offerta_domanda = 1;
                    merci_nave[i].scadenza = merci_porto[j].scadenza;
                    merci_nave[i].stato = Nella_Nave;
                    merci_porto[j].merce_consegnata =  merci_porto[j].quantita - merce_caricata_nave;
                    merci_porto[j].quantita = merci_porto[j].quantita - merce_caricata_nave;
                } 
                tim.tv_sec = (int) merce_caricata_nave / config_arr->SO_LOADSPEED;
                tim.tv_nsec = (merce_caricata_nave % config_arr->SO_LOADSPEED) * 1000000000L / config_arr->SO_LOADSPEED;
                nave->state = In_Porto_Per_Carico;
                if(merci_porto[j].quantita == 0) {
                    merci_porto[j].offerta_domanda = 2;
                    merci_porto[j].stato = Consegnata; /*merce[j] dovrebbe essere lo stesso id della merce [i]*/
                }
                nanosleep(&tim,NULL);
            }
            /*Scarica la nave, domanda il porto*/
            if(merci_nave[i].scadenza >= *giorni_simulazione && merci_nave[i].offerta_domanda == 1 && merci_porto[j].offerta_domanda == 0 && merci_nave[i].quantita > 0) {

                    nave->state = In_Porto_Per_Scarico;
                    merce_scaricata_nave = min(merci_nave[i].quantita,merci_porto[j].quantita);
                    tim.tv_sec = (int) merce_scaricata_nave / config_arr->SO_LOADSPEED;
                    tim.tv_nsec = (merce_scaricata_nave % config_arr->SO_LOADSPEED) * 1000000000L / config_arr->SO_LOADSPEED;
                    if(merce_scaricata_nave > 0) {
                        porto_vicino->merce_ricevuta = porto_vicino->merce_ricevuta + merce_scaricata_nave;
                        porto_vicino->merce_presente += merce_scaricata_nave;
                        merci_porto[j].merce_ricevuta += merce_scaricata_nave;
                        merci_nave[i].merce_consegnata = merci_nave[i].merce_consegnata +  merce_scaricata_nave;
                        merci_nave[i].quantita = merci_nave[i].quantita - merce_scaricata_nave;
                        merci_porto[j].quantita = merci_porto[j].quantita - merce_scaricata_nave;
                        if(merci_porto[j].quantita <= 0) {
                            merci_porto[j].stato = Consegnata;
                            merci_porto[j].offerta_domanda = 2;
                        }
                      
                        nanosleep(&tim,NULL);
                    } 
                
            } else if(merci_nave[i].quantita == 0 && merci_nave[i].scadenza >= *giorni_simulazione && merci_nave[i].offerta_domanda == 1) {
                merci_nave[i].stato = Consegnata;
                merci_nave[i].offerta_domanda = 2;
            }

            if(merci_porto[j].scadenza <= *giorni_simulazione && merci_porto[j].offerta_domanda == 1 && merci_porto[j].quantita > 0) {
                porto_vicino->merce_presente -= merci_porto[j].quantita;
                merci_porto[j].stato = Scaduta_Nel_Porto;
                merci_porto[j].offerta_domanda = 2;
                merci_porto[j].quantita = 0;
            }
            if(merci_nave[i].scadenza <= *giorni_simulazione && merci_nave[i].offerta_domanda == 1 && merci_nave[i].quantita > 0) {
                merci_nave[i].stato = Scaduta_Nella_Nave;
                merci_nave[i].quantita = 0;
                merci_nave[i].offerta_domanda = 2;
            }
           
        }
        unlock_shm(sem_id_nave);
        releaseSem(porto_vicino->sem_id_banchina,0);
        porto_vicino->banchine_occupate--;
    }
        check_port = 0;
        srand(getpid());
        nave->posizione.x = generate_random_double(0.0,config_arr->SO_LATO);
        nave->posizione.y = generate_random_double(0.0,config_arr->SO_LATO);
     /*In mare con carico*/
}

double calcola_distanza(Coordinates nave, Coordinates porto) {
    return abs(porto.x - nave.x) + abs(porto.y - nave.y);

}

int min(int x, int y) {
    return x > y ? y : x;
}


double generate_random_double(double min, double max) {
    double range = (max - min);
    double random_value = ((double) rand() / (double) RAND_MAX) * range + min;
    return random_value;
}

int calculate_total_cargo_weight(Merce* merci) {
    int i,j;
    int total_weight = 0;

    for(i = nave->offset_to_merci; i < nave->offset_to_merci + config_arr->SO_MERCI; i++) {
        total_weight += merci_nave[i].quantita;
    }
}


void clean() {
    int i;
    free(config_arr);
    shmdt(merci_nave);
    shmdt(giorni_simulazione);
    shmdt(merci_porto);
    shmdt(porti);
    shmdt(navi);  
    
}










