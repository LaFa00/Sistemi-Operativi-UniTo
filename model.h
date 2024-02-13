
#ifndef MODEL
#define MODEL

#define MERCI 10


/*Numero delle banchine occupate*/
int banchineOccupate;


typedef struct {
    int SO_NAVI;
    int SO_PORTI;
    int SO_MERCI;
    int SO_SIZE;
    int SO_MIN_VITA;
    int SO_MAX_VITA;
    double SO_LATO;
    int SO_SPEED;
    int SO_CAPACITY;
    int SO_BANCHINE;
    int SO_FILL;
    int SO_LOADSPEED;
    int SO_DAYS;

} Conf;


/*Struttura che rappresenta le coordinate*/
typedef struct {
    double x;
    double y;
} Coordinates;

typedef enum {
    Operativo,
    Attesa
}Stato_Porto;

typedef enum {
    Undefined,
    Nel_Porto, 
    Nella_Nave, 
    Consegnata, 
    Scaduta_Nel_Porto, 
    Scaduta_Nella_Nave,
    Richiesta_Nel_Porto,
    Richiesta_Nella_Nave
}Stato_Merce;

typedef struct {
    int id;
    int quantita; /*Quantita richiesta o offerta*/
    int merce_ricevuta;
    int scadenza;
    int scambi;
    int merce_consegnata;
    int offerta_domanda; /*0 = domanda, 1 = offerta*/
    Stato_Merce stato;
} Merce;

typedef struct {
    int id; /*identificatore*/
    int pid; /*identificatore*/
    int banchine;
    int sem_id_banchina;
    int disponibili;
    int banchine_occupate; /*Numero che indica banchine disponibili*/
    int offset_merci;
    int merce_spedita;
    int merce_ricevuta; /*Equivale a marcare la merce della nave che è stata consegnata*/
    int merce_presente;
    Stato_Porto stato;
    Coordinates posizione;
    Merce merci[MERCI];
} Porto;

/*Stati della nave*/
typedef enum {
    In_Mare,
    In_Viaggio,
    In_Mare_Vuota,
    In_Viaggio_Vuota,
    In_Porto,
    In_Porto_Per_Scarico,
    In_Porto_Per_Carico
} Stato_Nave;


/**/
typedef struct {
    int id;
    int pid;
    int capacity;
    int velocita;
    int merce_ricevuta;
    int merce_scaricata;
    Merce* merci;
    int offset_to_merci;
    Stato_Nave state;
    Coordinates posizione;
}Nave;

typedef struct {
    /*Merci*/
    int nave_in_mare_con_carico;
    int nave_in_mare_senza_carico;
    int nave_in_porto_per_carico;
    int nave_in_porto_per_scarico;
    int merce_porto_spedita;
    int merce_porto_presente;
    int merce_porto_ricevuta;
    int banchine_occupate;
}Report;

struct messagebuf {
    long int mType;
    char mText[1024];
}; /*messaggio con cui comunicheranno nave e porti */


struct message {
    long mtype;
    char mText[1024];
    int sem_id_banchina;
};
#endif
