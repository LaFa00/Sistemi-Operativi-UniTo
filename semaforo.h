

union semun {
    int              val;    /* Value for SETVAL */
    struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;  /* Array for GETALL, SETALL */
    struct seminfo  *__buf;  /* Buffer for IPC_INFO */
};

int initSemAvailable(int semId, int semNum);
int reserveSem(int semId, int semNum);
int releaseSem(int semId, int semNum);
int releaseSemIncrement(int semId, int semNum, int nIncrement);
void sync_process(int sem_id, int value);
void sync_msg_queue(int id_sem, int value);
int waitSem(int semId, int semNum);
int initSemAvailableTo0(int semId, int semNum);
void lock_shm(int sem_sync_id);
void unlock_shm(int sem_sync_id);
