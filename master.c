
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
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
#include "semaforo.h"




void stampa_porti();

void start_signals(){
    /* Struct per l'handling dei segnali e mask */
    struct sigaction sa;


    /* #### SETTING MASCHERA #### */ 
    /* Svuotamento della maschera */ 
    sigemptyset(&mask); 

    /* Aggiunta di SIGALRM nella mask. Questo per:
    -- gestire il trascorrere dei giorni
    -- terminare la simulazione dopo SO_DAYS  */ 
    sigaddset(&mask,SIGALRM);

    /* Aggiunta di SIGINT nella mask. Questo per interrompere
    la simulazione sotto comando dell'utente */ 
    sigaddset(&mask,SIGINT);


    /* #### SETTING E AVVIO SIGACTION #### */ 
    /* flag speciale in caso di cattura di un segnale durante una system call */ 
    sa.sa_flags = SA_RESTART;
    sa.sa_mask = mask;
    sa.sa_handler = handle_signals;

    /* Attivazione ascolto dei segnali con sigaction() */ 
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
}

void handle_signals(int sig){
    struct timespec tim1;
    
    sigprocmask(SIG_BLOCK, &mask, NULL); /* Blocco segnali nella mask */ 

    /* Switch per gestire il segnale ricevuto */ 
    switch(sig){
        /* Caso di SIGALRM */ 
        case SIGALRM:
            /* Stampa del daily report e aumento dei
            secondi rimanenti della simulazione */
            daily_report();
            printf("\nLa simulazione terminera' in: %d\n", *giorni_simulazione - 1);
            tim1.tv_sec = 1;
            tim1.tv_nsec = 0;
            nanosleep(&tim1,NULL);
            *giorni_simulazione -= 1;

            
            if(*giorni_simulazione <= 0){
                kill_all_process(); 
            } else if(check_merci_nave() && check_merci_porto()) {
                printf("Simulazione terminata per mancanza di merci\n");
                kill_all_process();
            }else{
                /* Reset alarm */ 
                alarm(1);
            }
            break;

        /* Caso di SIGINT */ 
        case SIGINT:
            alarm(0); /* Cancellazione del segnale corrente di alarm() */ 
            kill_all_process(); 
            clean();
            deallocate_IPCs();
            break;
        
        /* Caso di segnale non gestito */ 
        default:
            printf("Segnale anomalo non gestito.\n");
            break;
    }
    
    sigprocmask(SIG_UNBLOCK, &mask, NULL); /* Sblocco dei segnali nella mask */ 
}

int main(int argc, char const *argv[]) {
    set_conf();
    if(check_conf()) {
        init();    
        simulation();
    }
    return 0;
}


bool set_conf() {
    FILE *fp;
    char key[20];

    /*Apertura file config*/
    fp = fopen(CONFIG, "r");
    if(fp == NULL) {
        return false;
    }
    config = (Conf *)malloc(sizeof(Conf));
    if(config == NULL) {
        return false;
    }
    fscanf(fp,"%s %d\n", key, &config->SO_NAVI);
    fscanf(fp,"%s %d\n", key, &config->SO_PORTI);
    fscanf(fp,"%s %d\n", key, &config->SO_MERCI);
    fscanf(fp,"%s %d\n", key, &config->SO_SIZE);
    fscanf(fp,"%s %d\n", key, &config->SO_MIN_VITA);
    fscanf(fp,"%s %d\n", key, &config->SO_MAX_VITA);
    fscanf(fp,"%s %lf\n",key, &config->SO_LATO);
    fscanf(fp,"%s %d\n", key, &config->SO_SPEED);
    fscanf(fp,"%s %d\n", key, &config->SO_CAPACITY);
    fscanf(fp,"%s %d\n", key, &config->SO_BANCHINE);
    fscanf(fp,"%s %d\n", key, &config->SO_FILL);
    fscanf(fp,"%s %d\n", key, &config->SO_LOADSPEED);
    fscanf(fp,"%s %d\n", key, &config->SO_DAYS);

   


    /*Chiusura file config*/
    fclose(fp);
    return true;
}

bool check_conf() {
    if(config-> SO_LATO < 0) {
        printf("Il parametro SO_LATO e' negativo. Deve essere > 0 \n");
        return false;
    }

    if(config->SO_PORTI < 4) {
        printf("Il numero di porti deve essere >= 4\n");    
        return false;
    }

    if(config->SO_PORTI < 1) {
        printf("Il numero di porti deve essere >= 1\n");
        return false;
    }
    return true;
}



void init() {
    int i;
    
    
    srand(time(NULL));


    /*Allocazione array pid dei porti*/
    pid_port = (pid_t *)malloc(sizeof(pid_t) * config->SO_PORTI);

    pid_ship = (pid_t *)malloc(sizeof(pid_t) * config->SO_NAVI);

    /*Struct per mantenere le coordinate*/
    coord_port = (Coordinates*) malloc(sizeof(Coordinates) * config->SO_PORTI);

    /*Struct per mantenere coordinate navi*/
    coord_ship = (Coordinates*) malloc(sizeof(Coordinates) * config->SO_NAVI);


    
    
    gen_port_cells();
    gen_ship_cells();
    init_shared_memories();
    init_semaphores();
    init_message_queue();
    gen_port();
    gen_ship();
    start_signals();
}

void simulation() {
    struct sembuf sop;
    int child,status;
    int processi_terminati;
    int sem_num;
    int giorni = 0;
    int i;

    sop.sem_flg = 0;
    sop.sem_num = 0;
    sop.sem_op = 0; /*Settata a 0 serve per verificare se il valore del semaforo e' realmente zero*/
    while (semop(sem_id,&sop,1) == -1){
        perror("ERRORE SEMAFORO SINCRONIZZAZIONE");
        exit(EXIT_FAILURE);
    }
    printf("La Simulazione terminerà in %d\n", config->SO_DAYS);
    alarm(1);
    while ((child = wait(&status)) != -1) {
    }

    /*Stampa statistiche finali*/
    final_report();
    
    clean();
    deallocate_IPCs();
  
   
}

void gen_port() {
    int i;
    double r,c;
    pid_t port_pid; /*Variabile per il fork dei processi porto*/
    char *args[9];

   
    for(i = 0; i < config->SO_PORTI; i++) {
        r = coord_port[i].x;
        c =  coord_port[i].y;
        
        port_pid = fork();
        switch(port_pid) {
            case -1:
                perror("ERRORE DURANTE LA FORK()");
                exit(EXIT_FAILURE);
            case 0:
                /*Setting parametri*/
                args[0] = (char*)malloc(sizeof(char));
                snprintf(args[0],sizeof(args[0]),"%f",r);
                args[1] = (char *)malloc(sizeof(char));
                snprintf(args[1], sizeof(args[1]),"%f",c);
                args[2] = (char *)malloc(sizeof(char));
                snprintf(args[2],sizeof(args[2]),"%d",shm_master); /*ID Shared Memory*/
                args[3] = (char *)malloc(sizeof(char));
                snprintf(args[3],sizeof(args[3]),"%d",2);
                args[4] = (char *)malloc(sizeof(char));
                snprintf(args[4],sizeof(args[4]),"%d",port_id);
                args[5] = (char *)malloc(sizeof(char));
                snprintf(args[5],sizeof(args[5]),"%d",i);/*ID PORTO CORRENTE*/
                args[6] = (char *)malloc(sizeof(char));
                snprintf(args[6],sizeof(args[6]),"%d",sem_id);/*SEMAFORO SINCRONIZZAZIONE*/
                args[7] = (char *)malloc(sizeof(char));
                snprintf(args[7],sizeof(args[7]),"%d",msq_id); /*ID MESSAGE QUEUE*/
                args[8] = (char *)malloc(sizeof(char));
                snprintf(args[8],sizeof(args[8]),"%d",merci_id_porto);
                args[9] = (char *)malloc(sizeof(char));
                snprintf(args[9],sizeof(args[9]),"%d",giorni_simulazione_id);
                args[10] = NULL;
                execve("./porto",args,NULL);
                break;
            default:
                
                pid_port[i] = port_pid;
                break;

        }
        
    }
   
    
}

void gen_ship() {
    int i;
    double r,c;
    char *args[10];
    pid_t ship_pid;
    
    for(i = 0; i < config->SO_NAVI;i++) {
        r = coord_ship[i].x;
        c = coord_ship[i].y;
        ship_pid = fork();
        switch(ship_pid) {
            case -1:
                perror("ERRORE DURANTE LA FORK() in gen_ship()");
                exit(EXIT_FAILURE);
            case 0:
            
                args[0] = (char*)malloc(sizeof(char));
                snprintf(args[0],sizeof(args[0]),"%f",r);
                args[1] = (char *)malloc(sizeof(char));
                snprintf(args[1], sizeof(args[1]),"%f",c);
                args[2] = (char*)malloc(sizeof(char) * 12);
                snprintf(args[2],sizeof(args[2]),"%d",shm_master);
                args[3] = (char *)malloc(sizeof(char));
                snprintf(args[3],sizeof(args[3]),"%d",2);
                args[4] = (char *)malloc(sizeof(char));
                snprintf(args[4],sizeof(args[4]),"%d",port_id);
                args[5] = (char *)malloc(sizeof(char));
                snprintf(args[5],sizeof(args[5]),"%d",ship_id);
                args[6] = (char *)malloc(sizeof(char));
                snprintf(args[6],sizeof(args[6]),"%d",i);
                args[7] = (char *)malloc(sizeof(char));
                snprintf(args[7],sizeof(args[7]),"%d",sem_id);
                args[8] = (char *)malloc(sizeof(char));
                snprintf(args[8],sizeof(args[8]),"%d",msq_id);
                args[9] = (char *)malloc(sizeof(char));
                snprintf(args[9],sizeof(args[9]),"%d",merci_id_nave);
                args[10] = (char *)malloc(sizeof(char));
                snprintf(args[10],sizeof(args[10]),"%d",giorni_simulazione_id);
                args[11] = (char *)malloc(sizeof(char));
                snprintf(args[11],sizeof(args[11]),"%d",merci_id_porto);
                args[12] = NULL;
                execve("./nave", args,NULL);
                break;
            default:
                
                pid_ship[i] = ship_pid;
                break;



        }
    }
   

}









  


/*Generazione delle celle dei porti*/

void gen_port_cells() {
    double i, j;
    int count;
    
     
    srand(time(NULL));

    for(count = 0; count < config->SO_PORTI; count++) {
            if(count < 4) {
            coord_port[count].x = count % 2 == 0 ? 0 : config->SO_LATO - 1;
            coord_port[count].y = count < 2 ? 0 : config->SO_LATO - 1;
        } else {
            do {
                i = generate_random_double(0.0, config->SO_LATO);
                j = generate_random_double(0.0, config->SO_LATO);

            } while(is_coordinate_duplicate(coord_port,count,i,j));
            coord_port[count].x = i;
            coord_port[count].y = j;
        }

    }
}


void gen_ship_cells() {
   double i, j;
    int count = 0;


    while(count < config->SO_NAVI)  {
        i = generate_random_double(0.0, config->SO_LATO);
        j = generate_random_double(0.0, config->SO_LATO);

        if(coord_port[count].x != i && coord_port[count].y != j) {
            coord_ship[count].x = i;
            coord_ship[count].y = j;
        }
        count++;
    }
}










void init_shared_memories() {
    int i;
    int mem_port_size = sizeof(Porto) * config->SO_PORTI + sizeof(Merce) * config->SO_MERCI;
    init_shm_config();
    

    /*Shared memori Porto*/
    port_id = shmget(IPC_PRIVATE,mem_port_size, IPC_CREAT | 0666);
    if(port_id == -1){
        perror("ERRORE CREAZIONe MEMORIA CONDIVISA");
        exit(EXIT_FAILURE);
    }

    porti = shmat(port_id,NULL,0);
    if (porti == (void *) -1){
       perror("ERRORE ATTACH MEMORIA CONDIVISA");
        exit(EXIT_FAILURE);
    }

    ship_id = shmget(IPC_PRIVATE, sizeof(Nave) * config->SO_NAVI, IPC_CREAT | 0666);
    if(ship_id == -1){
        perror("ERRORE CREAZIONE MEMORIA NAVI");
        exit(EXIT_FAILURE);
    }

    navi = shmat(ship_id,NULL,0);
    if (navi == (void *) -1){
        perror("ERRORE ATTACH MEMORIA CONDIVISA");
        exit(EXIT_FAILURE);
    }

    merci_id_nave = shmget(IPC_PRIVATE, sizeof(Merce) * config->SO_MERCI * config->SO_NAVI, IPC_CREAT | 0666);
    if(merci_id_nave == -1){
        perror("ERRORE CREAZIONE MEMORIA MERCE");
        exit(EXIT_FAILURE);
    }

    merci_nave = (Merce*)shmat(merci_id_nave, NULL, 0);
    if (merci_nave == (void*)-1) {
        perror("Errore nell'attach della memoria condivisa per Merci");
        exit(EXIT_FAILURE);
    }

    merci_id_porto = shmget(IPC_PRIVATE, sizeof(Merce) * config->SO_MERCI * config->SO_PORTI, IPC_CREAT | 0666);
    if(merci_id_porto == -1){
        perror("ERRORE CREAZIONE MEMORIA MERCE");
        exit(EXIT_FAILURE);
    }

    merci_porto = (Merce*)shmat(merci_id_porto,NULL,0);
    if (merci_porto == (void*)-1) {
        perror("Errore nell'attach della memoria condivisa per Merci");
        exit(EXIT_FAILURE);
    }
    giorni_simulazione_id = shmget(IPC_PRIVATE,sizeof(int), IPC_CREAT | 0666);
    if(giorni_simulazione_id == -1){
        perror("ERRORE CREAZIONE MEMORIA GIORNI SIMULAZIONE");
        exit(EXIT_FAILURE);
    }

    giorni_simulazione = shmat(giorni_simulazione_id,NULL,0);
    if (giorni_simulazione == (void *) -1){
        perror("ERRORE ATTACH MEMORIA CONDIVISA");
        exit(EXIT_FAILURE);
    }
    *giorni_simulazione = config->SO_DAYS;


   

}

void init_shm_config() {
    Shared_Conf*shm_conf;
    size_t shared_mem_size;

    shared_mem_size = sizeof(Shared_Conf);

    shm_master = shmget(IPC_PRIVATE,shared_mem_size,IPC_CREAT | 0600);
    if(shm_master < 0) {
        perror("ERRORE CREAZIONe MEMORIA CONDIVISA");
        exit(EXIT_FAILURE);
    }
   
    /*Attacco segmento memoria condivisa*/
    shm_conf =  shmat (shm_master, NULL, 0);
    if(shm_conf == (void *) -1) {
        perror("ERRORE ATTACH MASTER MEMORIA CONDIVISA CONFIGURAZIONE");
        exit(EXIT_FAILURE);
    }
   

    shm_conf->config = config;
    memcpy(shm_conf, config, sizeof(Conf));
    shmdt((void*)shm_conf);
}





void init_semaphores() {
    int i;

    /* Set di semafori per la sincronizzazione fra processi (operazioni) */ 
    sem_id = semget(IPC_PRIVATE, 3, IPC_CREAT | 0600);
    if(sem_id == -1) {
        perror("Errore creazione semafori");
        exit(EXIT_FAILURE);
    }
    /* Semaforo per la sync iniziale */ 
    semctl(sem_id, 0, SETVAL, config->SO_PORTI + config->SO_NAVI); 
    /* Semaforo per la sync nelle msg queue */ 
    semctl(sem_id, 1, SETVAL, 1);
    /* Semaforo per la sync della memoria condivisa */ 
    semctl(sem_id, 2, SETVAL, 1);

}

void init_message_queue(){
    if((msq_id = msgget(IPC_PRIVATE, IPC_CREAT | 0666)) == -1) {
        perror("ERRORE CREAZIONEMESSAGE QUEUE");
        exit(EXIT_FAILURE);
    }
}

void daily_report() {
    int i,j;
    int nave_in_mare_con_carico = 0;
    int nave_in_mare_senza_carico = 0;
    int nave_in_porto_per_carico = 0;
    int nave_in_porto_per_scarico = 0;
    int merce_nella_nave = 0;
    int merce_scaduta_nella_nave = 0;
    int merce_nave_consegnata = 0;


    

    /*Numero di navi*/
    for(i = 0; i < config->SO_NAVI; i++) {
        if (navi[i].state == In_Mare) {
            nave_in_mare_con_carico++;
        }
        if (navi[i].state == In_Mare_Vuota) {
            nave_in_mare_senza_carico++;
        }
        if (navi[i].state == In_Porto_Per_Carico) {
            nave_in_porto_per_carico++;
        }
        if (navi[i].state == In_Porto_Per_Scarico) {
            nave_in_porto_per_scarico++;
        }
    }
    
    printf("---------------------------------------< REPORT DI GIORNATA %d >---------------------------------------\n", *giorni_simulazione);
	printf("--> Navi in mare con carico %d \n",nave_in_mare_con_carico);
	printf("--> Navi in mare senza carico %d \n", nave_in_mare_senza_carico);
	printf("--> Navi in porto per carico %d \n", nave_in_porto_per_carico);
	printf("--> Navi in porto per scarico %d \n", nave_in_porto_per_scarico);


    for(i = 0; i < config->SO_PORTI; i++) {
        printf("-> PORTO: %d ", porti[i].pid);
		printf("|-----> Qta merce presente : %d ", porti[i].merce_presente );
		printf("|-----> Qta merce ricevuta : %d ", porti[i].merce_ricevuta);
		printf("|-----> Qta merce spedita : %d ", porti[i].merce_spedita);
		printf("|-----> Banchine occupare/totali : %d\n", porti[i].banchine_occupate);
    }

    for(i = 0; i < config->SO_NAVI; i++) {
        printf("->NAVE: %d\n", navi[i].pid);
        for(j = navi[i].offset_to_merci; j < config->SO_MERCI + navi[i].offset_to_merci; j++) {
            if(merci_nave[j].stato == Scaduta_Nella_Nave) {
                merce_scaduta_nella_nave++;
            } else if(merci_nave[j].stato == Nella_Nave) {
                merce_nella_nave++;
            } else if(merci_nave[j].stato == Consegnata) {
                merce_nave_consegnata++;
            }
         }
            merce_nave_consegnata = merci_nave[j].merce_consegnata;
            printf("NAVE : %d MERCE PRESENTE: %d MERCE SCADUTA: %d MERCE CONSEGNATA: %d\n", navi[i].pid,merce_nella_nave, merce_scaduta_nella_nave, merce_nave_consegnata);
           
    }
    

    for(i = 0; i < config->SO_PORTI; i++) {
        printf("->PORTO: %d ", porti[i].pid);
        printf("TOTALE MERCE SPEDITA: %d ", porti[i].merce_spedita);
        printf("TOTALE MERCE RICEVUTA: %d ",porti[i].merce_ricevuta);
        printf("TOTALE MERCE PRESENTE: %d\n ", porti[i].merce_presente);
    }
    
    printf("-----------------------------------<FINE REPORT DI GIORNATA>---------------------------------------\n");
}

void final_report() {
    int i,j;
    Porto porto_maggior_offerta;
    Porto porto_maggiore_richiesta;
    int nave_in_mare_con_carico = 0;
    int nave_in_mare_senza_carico = 0;
    int nave_in_porto_per_carico = 0;
    int nave_in_porto_per_scarico = 0;
    int merce_nella_nave = 0;
    int merce_scaduta_nella_nave = 0;
    int merce_nave_consegnata = 0;
    int merce_nel_porto = 0;
    int merce_scaduta_nel_porto = 0;
    int merce_porto_consegnata = 0;

    for(i = 0; i < config->SO_NAVI; i++) {
        if (navi[i].state == In_Mare) {
            nave_in_mare_con_carico++;
        }
        if (navi[i].state == In_Mare_Vuota) {
            nave_in_mare_senza_carico++;
        }
        if (navi[i].state == In_Porto_Per_Carico) {
            nave_in_porto_per_carico++;
        }
        if (navi[i].state == In_Porto_Per_Scarico) {
            nave_in_porto_per_scarico++;
        }
    }

    printf("|--------------------------[ REPORT FINALE ]--------------------------|\n");
    printf("--> Navi in mare con carico %d \n",nave_in_mare_con_carico);
	printf("--> Navi in mare senza carico %d \n", nave_in_mare_senza_carico);
	printf("--> Navi in porto per carico %d \n", nave_in_porto_per_carico);
	printf("--> Navi in porto per scarico %d \n", nave_in_porto_per_scarico);

    
     for(i = 0; i < config->SO_NAVI; i++) {
        printf("->NAVE: %d\n ", navi[i].pid);
        for(j = navi[i].offset_to_merci; j < config->SO_MERCI + navi[i].offset_to_merci; j++) {
            if(merci_nave[j].stato == Scaduta_Nella_Nave) {
                merce_scaduta_nella_nave++;
            } else if(merci_nave[j].stato == Nella_Nave) {
                merce_nella_nave++;
            } else if(merci_nave[j].stato == Consegnata) {
                merce_nave_consegnata++;
            }
         }
            merce_nave_consegnata = merci_nave[j].merce_consegnata;
            printf("NAVE : %d MERCE PRESENTE: %d MERCE SCADUTA: %d MERCE CONSEGNATA: %d\n", navi[i].pid,merce_nella_nave, merce_scaduta_nella_nave, merce_nave_consegnata);
           
    }

    for(i = 0; i < config->SO_PORTI; i++) {
        printf("->PORTO: %d ", porti[i].pid);
        for(j = porti[i].offset_merci; j < porti[i].offset_merci + config->SO_MERCI; j++) {
           if(merci_porto[j].stato == Scaduta_Nel_Porto) {
                merce_scaduta_nella_nave++;
            } else if(merci_porto[j].stato == Nel_Porto) {
                merce_nella_nave++;
            } else if(merci_nave[j].stato == Consegnata) {
                merce_nave_consegnata++;
            }
            printf("PORTO : %d MERCE PRESENTE: %d MERCE SCADUTA: %d MERCE CONSEGNATA: %d\n", porti[i].pid,merce_nel_porto, merce_scaduta_nel_porto, merce_porto_consegnata);
        }
    }

    for(i = 0; i < config->SO_PORTI; i++) {
        if(porti[i].id == 0) {
            porto_maggior_offerta = porti[i];
        } else {
            if(porti[i].merce_spedita >= porto_maggior_offerta.merce_spedita) {
                porto_maggior_offerta = porti[i];
            }
        }
    }
    printf("Il porto che ha offerto il maggior numero di merci è: %d\n", porto_maggior_offerta.pid);

    for(i = 0; i < config->SO_PORTI; i++) {
        if(porti[i].id == 0) {
           porto_maggiore_richiesta = porti[i];
        } else {
            if(porti[i].merce_ricevuta >= porto_maggiore_richiesta.merce_ricevuta) {
                porto_maggiore_richiesta = porti[i];
            }
        }
    }
    printf("Il porto che ha richiesto il maggior numero di merci è: %d\n", porto_maggiore_richiesta.pid);

}

    
/*Generare le coordinate */
double generate_random_double(double min, double max) {
    double range = (max - min);
    double random_value = ((double) rand() / (double) RAND_MAX) * range + min;
    return random_value;
}

int is_coordinate_duplicate(Coordinates *port_coordinates, int count, int x, int y) {
    int i;
    for (i = 0; i < count; i++) {
        if (port_coordinates[i].x == x && port_coordinates[i].y == y) {
            return 1; 
        }
    }
    return 0; 
}

bool check_merci_nave() {
    int i,j;
    bool terminate = true;
    for(i = 0; i < config->SO_NAVI; i++) {
        for(j = navi[i].offset_to_merci; j < navi[i].offset_to_merci + config->SO_MERCI; j++) {
            if(merci_nave[j].offerta_domanda == 0 && merci_nave[j].quantita != 0) {
                return false;
            } else if(merci_nave[j].offerta_domanda == 1 && merci_nave[j].quantita != 0) {
                return false;
            }
        }
    }

    return terminate;
}


bool check_merci_porto() {
    int i,j;
    bool terminate = true;
    for(i = 0; i < config->SO_PORTI; i++) {
        for(j = porti[i].offset_merci; j < config->SO_MERCI + porti[i].offset_merci; j++) {
            if(merci_porto[j].offerta_domanda == 0 && merci_porto[j].quantita != 0) {
                return false;
            }else if(merci_porto[j].offerta_domanda == 1 && merci_porto[j].quantita != 0) {
                return false;
                
            }

        }
    }
    return terminate;
}

void clean() {
    int i,j;
    free(pid_port);
    free(pid_ship);
    free(coord_port);
    free(coord_ship);
    
    
}

void deallocate_IPCs() {
    
    shmdt(navi);
    shmdt(porti);
    shmdt(giorni_simulazione);
    shmdt(merci_porto);
    shmdt(merci_nave);

    shmctl(port_id,IPC_RMID,NULL);
    shmctl(ship_id,IPC_RMID,NULL);
    shmctl(shm_master,IPC_RMID,NULL);
    shmctl(shm_map,IPC_RMID,NULL);
    shmctl(merci_id_nave,IPC_RMID,NULL);
    shmctl(merci_id_porto,IPC_RMID,NULL);
    shmctl(giorni_simulazione_id,IPC_RMID,NULL);
    semctl(sem_id,0,IPC_RMID);
    semctl(sem_id,1,IPC_RMID);
    semctl(sem_id,2,IPC_RMID);
    msgctl(msq_id,IPC_RMID,NULL);
}



void kill_all_process(){
    int i;

    /* Killing dei processi source */ 
    for (i = 0; i < config->SO_NAVI; i++){
        kill(pid_ship[i], SIGQUIT);
    }
    
    /* Killing dei processi taxi */
    for (i = 0; i < config->SO_PORTI; i++){
        kill(pid_port[i], SIGQUIT);
    }
}

void stampa_porti() {
    int i,j;
    int somma_merci;
    bool somma = true;
    for(i = 0; i < config->SO_PORTI; i++) {
        for(j= 0; j < config->SO_MERCI; j++) {
            printf("PORTO: %d MERCE ID: %d QUANTITA: %d\n", porti[i].pid,merci_porto[j].id,merci_porto[j].quantita);
            somma_merci += merci_porto[j].quantita;
        }
    }
}

void stampa_navi() {
    int i,j;
    int offset;
    int offset_merce;
    for(i = 0; i < config->SO_NAVI; i++) {
        for(j = navi[i].offset_to_merci; j < navi[i].offset_to_merci + config->SO_MERCI; j++) {
            printf("Merce %d della nave %d - ID: %d, Quantità: %d\n", merci_nave[j].id,navi[i].pid, merci_nave[j].id, merci_nave[j].quantita);
        }
    }
}


