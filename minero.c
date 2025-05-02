/**
 * @file minero.c
 * @author Rodrigo Verde y Miguel Ruiz
 * @brief Definición de función minero y función auxiliar privada miner
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <mqueue.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <sys/stat.h>
#include "pow.h"
#include "minero.h"
#include "monitor1.h"
#include "utilities.h"
#include <time.h>


typedef struct {
    long ini;           /*!<Objetivo inicial para la búsqueda*/
    long nintentos;     /*!<Número de intentos que debe realizar*/
    long* objetivo;     /*!<puntero del objetivo a buscar*/
    long *resultado;    /*!<Puntero a la variable donde se guardará el resultado*/
    bool* found;        /*!<Puntero a un boolean que marca si se ha encontrado la solución*/
} Args;

/**
 * @brief Función encargada de buscar el resultado de la función hash
 * 
 * @param args Argumentos que se le pasan a cada hilo para poder realizar la búsqueda
 */
void *miner(void *args);


int minero(int nrondas, int lag) {
    pthread_t *hilos;       /*Hilos que se vana usar*/
    int i, j;
    int error;
    long nintentos;         /*Intentos que realiza el miner para buescar*/
    Args *arg;              /*Estructuras de argumentos que se van a pasar por los hilos*/
    bool found;             /*Variable donde se guarda si se ha encontrado la solución*/
    int status;
    message msg;
    struct mq_attr attributes;
    mqd_t queue;

    attributes.mq_maxmsg = N_MSG;
    attributes.mq_msgsize = sizeof(message);
    queue = mq_open(MQ_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &attributes);
    
    srand(time(NULL));
    msg.target = rand() % POW_LIMIT;
    msg.finish = false;



    /*Reserva de memoria para los hilos*/
    hilos = malloc(N_HILOS * sizeof(pthread_t));
    if (!hilos) {
        perror("Error reservando memoria para hilos");
        exit(EXIT_FAILURE);
    }

    /*Reserva de memoria para los argumentos*/
    arg = malloc(N_HILOS * sizeof(Args));
    if (!arg) {
        perror("Error reservando memoria para argumentos");
        free(hilos);
        exit(EXIT_FAILURE);
    }

    /*Se divide el espacio de búsqueda*/
    nintentos = POW_LIMIT / N_HILOS;


    fprintf(stdout, "[%d] Generating blocks...\n", getpid());
    fflush(stdout);

    for (i = 0; i < nrondas; i++) {
        found = false;

        /*Rellenamos la información de los argumentos que se pasarán al miner*/
        for (j = 0; j < N_HILOS; j++) {
            arg[j].ini = nintentos * j;

            if (j != N_HILOS - 1){
                arg[j].nintentos = nintentos;
            }else{
                arg[j].nintentos = (POW_LIMIT - j * nintentos);
            }

            arg[j].found = &found;
            arg[j].objetivo = &msg.target;
            arg[j].resultado = &msg.result;

            /*Llamada al miner en un hilo*/
            error = pthread_create(&hilos[j], NULL, miner, &arg[j]);
            if (error != 0) {
                free(hilos);
                free(arg);
                fprintf(stderr, "pthread_create: %s\n", strerror(error));
                exit(EXIT_FAILURE);
            }
        }

        /*Recogida de todos los hilos*/
        for (j = 0; j < N_HILOS; j++) {
            error = pthread_join(hilos[j], NULL);
            if (error != 0) {
                free(hilos);
                free(arg);
                fprintf(stderr, "pthread_join: %s\n", strerror(error));
                exit(EXIT_FAILURE);
            } 
        }

        if (i == nrondas - 1) {
            msg.finish = true;
        }

        mq_send(queue, (char*)&msg, sizeof(message), 0);
        esperar_milisegundos(lag);
        msg.target = msg.result;
    }

    fprintf(stdout, "[%d] Finishing...\n", getpid());
    fflush(stdout);

    /*Cierre de tuberías y liberación de memoria*/
    mq_close(queue);
    mq_unlink(MQ_NAME);
    free(hilos);
    free(arg);

    wait(&status);

    exit(EXIT_SUCCESS);
}




void *miner(void *args) {
    int i;
    long result;
    Args* arg = (Args*)args;    /*Casting del puntero void a Args*/

    /*Bucle buscando la solución*/
    for (i = 0; i < arg->nintentos; i++) {
        if (*arg->found) {
            return NULL;
        }
        
        result = pow_hash(arg->ini + i);
        
        if (result  == *arg->objetivo) {
            *arg->found = true;
            *arg->resultado = arg->ini+i;
            return NULL;
        }
    }
    return NULL;
}