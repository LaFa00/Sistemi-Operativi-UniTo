#define _GNU_SOURCES

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>   
#include <sys/shm.h>  
#include <sys/msg.h>   
#include <errno.h>
#include <fcntl.h>      
#include <signal.h>
#include <stdbool.h>
#include <time.h>
#include "porto.h"
#include <sys/sem.h>
#include "semaforo.h"

void stampa_porti();

void start_signals(){
    /* Struct per l'handling dei segnali */
    struct sigaction sa;

    /* #### SETTING MASCHERA #### */ 
    sigemptyset(&mask); /* Svuotamento della maschera */ 

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


    /* #### SETTING SIGACTION #### */ 
    sa.sa_flags = SA_RESTART;
    sa.sa_mask = mask;
    sa.sa_handler = handle_signals;
    /* Starting della sigaction */
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGINT,&sa,NULL);
}



void handle_signals(int sig){
    sigprocmask(SIG_BLOCK, &mask, NULL); /* Blocco segnali nella mask */ 

    /* Switch per gestire il segnale ricevuto */ 
    switch (sig){
        /* Caso di SIGINT (terminazione simulazione) */ 
        case SIGQUIT:
            /* Free delle risorse */ 
            clean();
            exit(EXIT_SUCCESS);
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
    int i;
    int indice_porto;
    if(argc < 10) {
        perror("Numero parametri non valido\n");
        exit(EXIT_FAILURE);
    }
    start_signals();
    init_shared_memories(argv);
    init_message_queue(atoi(argv[7]));
    init_semaphores(atoi(argv[6]));
    initialize_port(argv);
    sync_process(sem_id_porto,-1);
    while(1) {
       comunicazione_nave(1);
        
    }
    return(-1);
}

void init_shared_memories(char *argv[]) {
   
    initialize_config(atoi(argv[2])); /*ID SHARED MEMORY*/
    porti = shmat(atoi(argv[4]),NULL,0);
    if (porti == (void *) -1){
       perror("ERRORE ATTACH MEMORIA CONDIVISA");
        exit(EXIT_FAILURE);
    }

    merci_id_porto = atoi(argv[8]);
    
    merci_porto = shmat(merci_id_porto,NULL,0);
    if (merci_porto == (void *) -1){
       perror("ERRORE ATTACH MEMORIA CONDIVISA");
        exit(EXIT_FAILURE);
    }

}

void initialize_config(int sharedId) {
    Conf *shm_ptr;
    config_arr = (Conf*)malloc(sizeof(Conf));
    shm_ptr = (Conf *)shmat(sharedId,NULL,0);
    if(shm_ptr == (void *) -1) {
        perror("ERRORE ATTACH MEMORIA CONDIVISA CONFIGURAZIONE");
        exit(EXIT_FAILURE);
    }
    memcpy(config_arr,shm_ptr,sizeof(Conf));
    shmdt((void *) shm_ptr);
}



void initialize_port(char *argv[]) {
    int i;
    int min= 1;
    int quantita_richiesta;
    int quantita_offerta;
    int offset_merce;
    int indice_offset;
    int capacita = config_arr->SO_FILL / config_arr->SO_PORTI; /*Capacità massima di tonnellate di merci per porto*/
    int capacita_media_porto = capacita / config_arr->SO_MERCI;
    int capacita_rimanente = capacita_media_porto * config_arr->SO_MERCI;
    int quantita;
    int somma_merci_offerte = 0;
    int somma_merci_richieste = 0;
    int totale_offerta,totale_domanda;
 
    r_port = atof(argv[0]);
    c_port = atof(argv[1]);
    indice_porto = atoi(argv[5]);
    lock_shm(sem_id_porto);
    porti[indice_porto].id = indice_porto;
    porti[indice_porto].pid = getpid();
    porti[indice_porto].posizione.x = r_port;
    porti[indice_porto].posizione.y = c_port;
    porti[indice_porto].merce_presente = 0;
    porti[indice_porto].merce_ricevuta = 0;
    porti[indice_porto].merce_spedita = 0;
    generate_banchine(config_arr->SO_BANCHINE,indice_porto);
    porti[indice_porto].disponibili = porti[indice_porto].banchine;
    offset_merce = indice_porto * config_arr->SO_MERCI;
    porti[indice_porto].offset_merci = offset_merce;
    /*Inizializzazione delle merci*/
    totale_offerta = capacita / 2;
    totale_domanda = capacita / 2;
    for(i = 0; i < config_arr->SO_MERCI; i++) {
        srand(clock() ^ getpid());
        indice_offset = offset_merce + i;
        merci_porto[indice_offset].id = i;
        merci_porto[indice_offset].offerta_domanda = rand() % 2;
        if(merci_porto[indice_offset].offerta_domanda == 1) {
            if(totale_offerta > 0) {
                merci_porto[indice_offset].quantita =  1 + rand() % (config_arr->SO_SIZE);
                merci_porto[indice_offset].scadenza = rand() % (config_arr->SO_MAX_VITA - config_arr->SO_MIN_VITA + 1) + config_arr->SO_MIN_VITA;
                merci_porto[indice_offset].stato = Nel_Porto;
                if(merci_porto[indice_offset].quantita > totale_offerta) {
                    merci_porto[indice_offset].quantita = totale_offerta;
                    merci_porto[indice_offset].stato = Nel_Porto;
                }
                totale_offerta -= merci_porto[indice_offset].quantita;
                } else {
                    merci_porto[indice_offset].offerta_domanda = 0;
                    merci_porto[indice_offset].quantita =  1 + rand() % (config_arr->SO_SIZE);
                    merci_porto[indice_offset].scadenza = 0;
                    merci_porto[indice_offset].stato = Richiesta_Nel_Porto;
                    totale_domanda -= merci_porto[indice_offset].quantita;
                }
        } else {
            if(totale_domanda > 0) {
                merci_porto[indice_offset].quantita =  1 + rand() % (config_arr->SO_SIZE);
                merci_porto[indice_offset].scadenza = 0;
                if(merci_porto[indice_offset].quantita > totale_domanda) {
                    merci_porto[indice_offset].quantita = totale_domanda;
                    merci_porto[indice_offset].scadenza = 0;
                    merci_porto[indice_offset].stato = Richiesta_Nel_Porto;
                }
                totale_domanda -= merci_porto[indice_offset].quantita;
            }
        }
    }
    for(i = porti[indice_porto].offset_merci; i < config_arr->SO_MERCI + porti[indice_porto].offset_merci; i++) {
        if(merci_porto[i].offerta_domanda == 1) {
            porti[indice_porto].merce_presente = porti[indice_porto].merce_presente + merci_porto[i].quantita;
        }
            
    }
    unlock_shm(sem_id_porto);
    
}



void init_semaphores(int sem_id) {
    int i;
    sem_id_porto = sem_id;
    if(sem_id_porto == -1) {
        perror("Errore creazione semaforo");
        exit(EXIT_FAILURE);
    }
}

void init_message_queue(int msq_id) {
    
    if((msqid_porto = msq_id)== -1) {
        perror("ERRORE CREAZIONE MESSAGE QUEUE NAVE");
        exit(EXIT_FAILURE);
    }
}

/*Genera il numero di banchine*/
void generate_banchine(int num_banchine,int indice_porto) {
    int i;
    int banchine;
    srand(getpid()); /*Dovrebbe essere unico per ogni processo*/
    banchine = (rand () % config_arr->SO_BANCHINE) + 1;
    porti[indice_porto].banchine = banchine;
    
    /*Genero semaforo per le banchine*/
    sem_id_banchine = semget(IPC_PRIVATE,1, IPC_CREAT | 0666);
    
    if (sem_id_banchine == -1) {
        perror("Errore nella creazione degli insiemi di semafori");
        exit(EXIT_FAILURE);
    }
    semctl(sem_id_banchine,0,SETVAL,banchine);
   
    porti[indice_porto].sem_id_banchina = sem_id_banchine;
 
}



void comunicazione_nave(int num_sem_banchina) {
    int i;
    struct message richiesta,risposta;
    Merce* merci_nave;
    Merce merce;
    Merce merciaData;
    int sep;
    int quantita_merce,id_merce,_merce,scambi_merce,stato,offerta_domanda;
    char tmp[20];
    char *token;
    int is_not;
    char *rest;
    const char delimiter[2] = ";";
    int skip = 0;


    srand(getpid());
    
    
    sync_msg_queue(sem_id_porto,1);
        if(msgrcv(msqid_porto,&richiesta,sizeof(richiesta.mText), getpid(),0) == -1) {
            if(errno == ENOMSG) {
                printf("NESSUN MESSAGGIO NELLA CODA\n");

            }
        }
    sync_msg_queue(sem_id_porto,-1);
    risposta.mtype = getpid();

    if(porti[indice_porto].pid == richiesta.mtype) {
        if(semctl(sem_id_banchine,0,GETVAL) > 0) {
            strcpy(risposta.mText,"ATTRACCO CONSENTITO");
            porti[indice_porto].banchine_occupate++;
        } else {
            strcpy(risposta.mText,"ATTRACCO NON CONSENTITO");
        }
    }

    sync_msg_queue(sem_id_porto,1);
    if(msgsnd(msqid_porto,&risposta,sizeof(risposta.mText),IPC_NOWAIT) == -1) {
        perror("ERRORE INVIO PORTO");
    }
    sync_msg_queue(sem_id_porto,-1);

    if(semctl(porti[indice_porto].sem_id_banchina,0,GETVAL) > 0) {
            porti[indice_porto].banchine_occupate--;
    } else if(semctl(porti[indice_porto].sem_id_banchina,0,GETVAL) == 0) {
            porti[indice_porto].banchine_occupate = 0;
    } else {
        perror("ERRORE BANCHINE PORTO SEMAFORO NEGATIVO\n");
        exit(EXIT_FAILURE);
    }
}



void clean() {
    shmdt(merci_porto);
    shmdt(porti);
    semctl(sem_id_banchine,0,IPC_RMID);
    free(config_arr);
    
}













