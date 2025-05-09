/**
 * @file minero.c
 * @author Rodrigo Verde y Miguel Ruiz
 * @brief Definición de función minero y función auxiliar privada miner
 *
 */
#define _POSIX_C_SOURCE 200809L
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
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>



#include "pow.h"
#include "minero.h"
#include "monitor1.h"
#include "utilities.h"


static volatile sig_atomic_t alarm_signal = 0;
static volatile sig_atomic_t usr1_signal = 0;
static volatile sig_atomic_t usr2_signal = 0;

typedef struct {
    long ini;           /*!<Objetivo inicial para la búsqueda*/
    long nintentos;     /*!<Número de intentos que debe realizar*/
    long* objetivo;     /*!<puntero del objetivo a buscar*/
    long *resultado;    /*!<Puntero a la variable donde se guardará el resultado*/
    bool* found;        /*!<Puntero a un boolean que marca si se ha encontrado la solución*/
} Args;

void handler(int sig) {
    if (sig == SIGALRM || sig == SIGINT) {
        alarm_signal = 1;
    } 
    else if (sig == SIGUSR1) {
        usr1_signal = 1;
    }
    else if (sig == SIGUSR2) {
        usr2_signal = 1;
        printf("Señal 2\n");
        fflush(stdout);
    }
}

/*Código para la siguiente ronda*/
void next_round(Mem_Sys *data){
    int i;
    
    /*Enviar SIGUSR1 a los procesos mienro*/
    for (i = 0; i < (data->mineros -1) && i < MAX_PIDS; ){
        if (data->pids[i] != getgid() && data->pids[i] != 0)
        {
            kill(data->pids[i], SIGUSR1);
            i++;
        }
    }
    kill(getpid(), SIGUSR1);

}
/**
 * @brief Función encargada de buscar el resultado de la función hash
 * 
 * @param args Argumentos que se le pasan a cada hilo para poder realizar la búsqueda
 */
void *miner(void *args);


void start_mining(int threads, Mem_Sys *data, mqd_t queue);


void ganador(Mem_Sys *data, int resultado, mqd_t queue);

void perdedor(Mem_Sys *data);

void terminar(Mem_Sys *data, mqd_t queue);


int minero(int seconds, int threads, Mem_Sys *data) {
    struct mq_attr attributes;
    mqd_t queue;
    struct sigaction act;


    /*Configuración de sigaction*/
    act.sa_handler = handler;
    act.sa_flags = 0;
    sigemptyset(&(act.sa_mask));
    
    if (sigaction(SIGUSR1, &act, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    
    if (sigaction(SIGALRM, &act, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGUSR2, &act, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }


    /*Activar alarma*/
    if (alarm(seconds)) {
        fprintf(stderr, "Fallo al configurar la alarma\n");
    }

  
    attributes.mq_maxmsg = N_MSG;
    attributes.mq_msgsize = sizeof(Bloque);
    queue = mq_open(MQ_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &attributes);

    if (data->primero == getpid()) {
        esperar_milisegundos(100);
        next_round(data);
    }
    
    while (1) {
        esperar_milisegundos(100);
        if (usr1_signal == 1) {
            start_mining(threads, data, queue);
        }
        usr2_signal = 0;
        if (alarm_signal == 1) {
            terminar(data, queue);
        }
    }
        


    exit(EXIT_SUCCESS);
}

void start_mining(int threads, Mem_Sys *data, mqd_t queue){
    int n_intentos;
    bool found;
    Args *arg;              /*Estructuras de argumentos que se van a pasar por los hilos*/
    pthread_t *hilos;       /*Hilos que se vana usar*/
    int j;
    int error;
    long resultado=0;

    usr1_signal = 0;

    /*Reserva de memoria para los hilos*/
    hilos = malloc(threads * sizeof(pthread_t));
    if (!hilos) {
        perror("Error reservando memoria para hilos");
        exit(EXIT_FAILURE);
    }

    /*Reserva de memoria para los argumentos*/
    arg = malloc(threads * sizeof(Args));
    if (!arg) {
        perror("Error reservando memoria para argumentos");
        free(hilos);
        exit(EXIT_FAILURE);
    }
    /*Se divide el espacio de búsqueda*/
    n_intentos = POW_LIMIT / threads;
        
        found = false;

        /*Rellenamos la información de los argumentos que se pasarán al miner*/
        for (j = 0; j < threads; j++) {
            arg[j].ini = n_intentos * j;

            if (j != threads - 1){
                arg[j].nintentos = n_intentos;
            }else{
                arg[j].nintentos = (POW_LIMIT - j * n_intentos);
            }

            arg[j].found = &found;
            sem_wait(&data->memory);
            arg[j].objetivo = &data->actual.target;
            sem_post(&data->memory);
            arg[j].resultado = &resultado;

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
        for (j = 0; j < threads; j++) {
            error = pthread_join(hilos[j], NULL);
            if (error != 0) {
                free(hilos);
                free(arg);
                fprintf(stderr, "pthread_join: %s\n", strerror(error));
                exit(EXIT_FAILURE);
            } 
        }


        free(hilos);
        free(arg);
        if (alarm_signal == 1) {
            terminar(data, queue);
        }
        else if (usr2_signal == 1) {
            perdedor(data);
        }
        else{
            if (sem_trywait(&data->ganador) == 0){
            ganador(data, resultado, queue);
            }else{
                while (1) {
                    esperar_milisegundos(50);
                    if (usr2_signal == 1) {
                        perdedor(data);
                    }
                }
            }
            
        }
}



void *miner(void *args) {
    int i;
    long result;
    Args* arg = (Args*)args;    /*Casting del puntero void a Args*/

    /*Bucle buscando la solución*/
    for (i = 0; i < arg->nintentos && alarm_signal == 0 && usr2_signal == 0; i++) {
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

void ganador(Mem_Sys *data, int resultado, mqd_t queue){
    int i, j;
    int votos_favor=0;

    /*Registra el resultado y manda la señal a todos los procesos minero*/
    sem_wait(&data->memory);
    data->actual.result = resultado;
    data->actual.pid = getpid();

    for (i = 0; i < (data->mineros -1) && i < MAX_PIDS ; ){
        if (data->pids[i] != getgid() && data->pids[i] != 0) {
            kill(data->pids[i], SIGUSR2);
            i++;
        }
    }

    printf("N miners: %d\n", data->mineros);
    fflush(stdout);
    sem_post(&data->memory);

    /*Cuenta los votos introducidos*/
    for ( i = 0; i < MAX_VOT; i++) {

        esperar_milisegundos(50);
        sem_wait(&data->memory);

        if (data->cont_votos == (data->mineros -1)) {
            break;
        }
        sem_post(&data->memory);
    }

    sem_post(&data->memory);
    /*Cuenta y guarda los resultados en el bloque*/
    sem_wait(&data->memory);

    for ( i = 0; i < data->cont_votos; i++) {
        if (data->votos[i] == true) {
            votos_favor++;
        }
    }

    data->actual.votos_tot = data->cont_votos;
    data->actual.votos_pos = votos_favor;
    if (votos_favor >= data->cont_votos/2) {
        for ( i = 0; i < MAX_PIDS; i++) {
            if (data->carteras[i].pid == getpid()) {
                data->carteras[i].monedas += 1;
            }
            
        }
    }    

    for ( j = 0, i = 0; i < data->mineros; i++) {
        if (data->carteras[i].pid != 0) {
            data->actual.carteras[j].pid = data->carteras[i].pid;
            data->actual.carteras[j].monedas = data->carteras[i].monedas;
            j++;
        }
        
    }
    sem_post(&data->memory);

    mq_send(queue, (char*)&data->actual, sizeof(Bloque), 0);

    

    sem_wait(&data->memory);
    data->ultimo = data->actual;
    data->actual.target = data->ultimo.result;
    data->actual.id_bloque ++;

    sem_post(&data->memory);
    sem_post(&data->ganador);

    next_round(data);
}


void perdedor(Mem_Sys *data){
    
    printf("Perdedor\n");
    fflush(stdout);


    srand(time(NULL)); 
    sem_wait(&data->memory);
    if (rand() % 2) {
        data->votos[data->cont_votos]= true;
    }else {
        data->votos[data->cont_votos]= false;
    }
    data->cont_votos++;

    sem_post(&data->memory);

    usr2_signal = 0;
}

void terminar(Mem_Sys *data, mqd_t queue){
    int i;
    
    sem_wait(&data->memory);
    for ( i = 0; i < MAX_PIDS; i++) {
        if (data->pids[i] == getpid()) {
            data->pids[i] = 0;
        }
    }

    for ( i = 0; i < MAX_PIDS; i++) {
        if (data->carteras[i].pid == getpid()) {
            data->carteras[i].pid = 0;
            data->carteras[i].monedas = 0;
        }
    }

    data->mineros--;
    if (data->mineros != 0) {
        sem_post(&data->memory);
        exit(EXIT_SUCCESS);
    }

    sem_post(&data->memory);

    sem_destroy(&data->memory);
    sem_destroy(&data->ganador);

    data->actual.finish= true;
    mq_send(queue, (char*)&data->actual, sizeof(Bloque), 0);
    munmap(data, sizeof(Mem_Sys));
    mq_close(queue);
    
    mq_unlink(MQ_NAME);
    shm_unlink(SHM_NAME);
    exit(EXIT_SUCCESS);
}