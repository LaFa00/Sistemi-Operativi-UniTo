#include "semaforo.h"
#include <sys/types.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>


int initSemAvailable(int semId, int semNum) {
    union semun arg;
    arg.val = 1;
    return semctl(semId, semNum, SETVAL, arg);
}

int initSemAvailableTo0(int semId, int semNum) {
    union semun arg;
    arg.val = 0;
    return semctl(semId, semNum, SETVAL, arg);
}


/* Reserve semaphore - decrement it by 1 */
int reserveSem(int semId, int semNum) {
    struct sembuf sops;
    sops.sem_num = semNum;
    sops.sem_op = -1;
    sops.sem_flg = 0;
    return semop(semId, &sops, 1);
}
/* Release semaphore - increment it by 1 */
int releaseSem(int semId, int semNum) {
    struct sembuf sops;
    sops.sem_num = semNum;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    return semop(semId, &sops, 1);
}
int releaseSemIncrement(int semId, int semNum, int nIncrement) {
    struct sembuf sops;
    sops.sem_num = semNum;
    sops.sem_op = nIncrement;
    sops.sem_flg = 0;
    return semop(semId, &sops, 1);
}

int waitSem(int semId, int semNum) {
    struct sembuf sops;
    sops.sem_num = semNum;
    sops.sem_op = 0;
    sops.sem_flg = 0;
    return semop(semId, &sops, 1);
}

void lock_shm(int sem_sync_id){
    struct sembuf sop; /* Struct per le semop() */

    /* Setting della struct */ 
    sop.sem_flg = 0;
    sop.sem_num = 2; /* Numero semaforo nel set */ 
    sop.sem_op = -1;

    /* Operazione */
    while (semop(sem_sync_id,&sop,1) == -1){
        if(errno != EINTR){
            perror("ERRORE P() SEMAFORO MEMORIA CONDIVISA");
            exit(EXIT_FAILURE);
        }
    }
}


void unlock_shm(int sem_sync_id){
    struct sembuf sop; /* Struct per le semop() */

    /* Setting della struct */ 
    sop.sem_flg = 0;
    sop.sem_num = 2; /* Numero semaforo nel set */ 
    sop.sem_op = 1;

    /* Operazione */ 
    
    while (semop(sem_sync_id,&sop,1) == -1){
        if(errno != EINTR){
            perror("ERRORE V() SEMAFORO MEMORIA CONDIVISA");
            exit(EXIT_FAILURE);
        }
    }
}

 void sync_process(int sem_id, int value) {
        /* Struct per le semop() */ 
        struct sembuf sop;

        /* Var per il check del valore del semaforo */ 
        int sem_stat = semctl(sem_id,0,GETVAL);

        /* Se il semaforo ha un valore > 0, significa che dei
        processi devono ancora fare l'operazione di P() sul
        semaforo e attendere di sincronizzarsi con tutti gli
        altri */ 
        if (sem_stat > 0){
            /* Setting della struct */ 
            sop.sem_num = 0;
            sop.sem_flg = 0;
            sop.sem_op = value;
            
            /* Operazione di P() */
            while(semop(sem_id,&sop,1) == -1) {
                if(errno != EINTR){
                    perror("ERRORE P()");
                    exit(EXIT_FAILURE);
                }
            } 

            /* Attesa che il semaforo diventi 0, a fine di 
            sync con gli altri processi */ 
            
            sop.sem_op = 0;
            while(semop(sem_id,&sop,1) == -1) {
                if(errno != EINTR){
                    perror("ERRORE V()");
                    exit(EXIT_FAILURE);
                }
            }
            
        }
 }
        
void sync_msg_queue(int id_sem, int value){
    /* Struct per le semop() */ 
    struct sembuf sop;

    /* Setting della struct */ 
    sop.sem_num = 1;
    sop.sem_flg = 0;
    sop.sem_op = value;

    /* Operazione */
    while(semop(id_sem,&sop,1) == -1) {
        if(errno != EINTR){
            perror("ERRORE SEMOP MESSAGE QUEUE");
            exit(EXIT_FAILURE);
        }
    } 
}
