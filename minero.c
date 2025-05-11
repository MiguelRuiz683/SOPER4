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
#include "registrador.h"


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
        
        
    }
}

/*Código para la siguiente ronda*/
void next_round(Mem_Sys *data) {
    int i;
    int j;
    sem_wait(&data->iniciar);
    sem_wait(&data->memory);
    
    data->cont_votos = 0;
    memset(data->votos, 0, sizeof(data->votos));
    
    for ( i = 0, j=0; i < MAX_PIDS && j < (data->mineros - 1); i++) {
        if (data->pids[i] != 0 && data->pids[i] != getpid()) {
            kill(data->pids[i], SIGUSR1);
            j++;
        }
    }
    
    sem_post(&data->memory);
    
    kill(getpid(), SIGUSR1);
}
/**
 * @brief Función encargada de buscar el resultado de la función hash
 * 
 * @param args Argumentos que se le pasan a cada hilo para poder realizar la búsqueda
 */
void *miner(void *args);


int start_mining(int threads, Mem_Sys *data, mqd_t queue, int *fd);


void ganador(Mem_Sys *data, int resultado, mqd_t queue);

void perdedor(Mem_Sys *data);

void terminar(Mem_Sys *data, mqd_t queue,int  *fd);

void abandonar_sistema(Mem_Sys *data);


int minero(int seconds, int threads, Mem_Sys *data, int *fd) {
    struct mq_attr attributes;
    mqd_t queue;
    struct sigaction act;

    srand(time(NULL) ^ getpid());

    /*Configuración de sigaction*/
    act.sa_handler = handler;
    act.sa_flags = 0;
    sigemptyset(&(act.sa_mask));

    attributes.mq_maxmsg = N_MSG;
    attributes.mq_msgsize = sizeof(Bloque);
    queue = mq_open(MQ_NAME, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &attributes);

    if (queue == (mqd_t)-1) {
        abandonar_sistema(data);
    
        if (data->mineros == 0) {
            sem_destroy(&data->memory);
            sem_destroy(&data->ganador);
            sem_destroy(&data->iniciar);
        }
    
        while (data->mineros > 0) {
            esperar_milisegundos(100);
        }

        munmap(data, sizeof(Mem_Sys));

        perror("Error al abrir la cola de mensajes");
        return EXIT_FAILURE;
    }
    
    if (sigaction(SIGUSR1, &act, NULL) < 0) {
        terminar(data, queue, fd);
        perror("sigaction");
        return EXIT_FAILURE;
    }
    
    if (sigaction(SIGALRM, &act, NULL) < 0) {
        terminar(data, queue, fd);
        perror("sigaction");
        return EXIT_FAILURE;
    }

    if (sigaction(SIGINT, &act, NULL) < 0) {
        terminar(data, queue, fd);
        perror("sigaction");
        return EXIT_FAILURE;
    }

    if (sigaction(SIGUSR2, &act, NULL) < 0) {
        terminar(data, queue, fd);
        perror("sigaction");
        return EXIT_FAILURE;
    }

    /*Activar alarma*/
    if (alarm(seconds)) {
        fprintf(stderr, "Fallo al configurar la alarma\n");
        terminar(data, queue, fd);
        return EXIT_FAILURE;
    }

    sem_post(&data->iniciar);
    if (data->primero == getpid()) {
        esperar_milisegundos(100);
        next_round(data);
    }
    
    while (1) {
        esperar_milisegundos(100);
        if (usr1_signal == 1) {
            if (start_mining(threads, data, queue, fd) == EXIT_FAILURE) {
                terminar(data, queue, fd);
                return EXIT_FAILURE;
            }
        }


        usr2_signal = 0;
        if (alarm_signal == 1) {
            terminar(data, queue, fd);
            break;
        }
    }

    return EXIT_SUCCESS;
}

int start_mining(int threads, Mem_Sys *data, mqd_t queue, int *fd) {
    int n_intentos;
    bool found;
    Args *arg;
    pthread_t *hilos;
    int j, error;
    long resultado = 0;
    int timeout = 0;
    int nbytes;

    usr1_signal = 0;  

    /* Reserva de memoria para hilos y argumentos */
    hilos = malloc(threads * sizeof(pthread_t));
    if (!hilos) {
        perror("Error reservando memoria para hilos");
        return EXIT_FAILURE;
    }

    arg = malloc(threads * sizeof(Args));
    if (!arg) {
        perror("Error reservando memoria para argumentos");
        free(hilos);
        return EXIT_FAILURE;
    }

    /* Dividir espacio de búsqueda */
    n_intentos = POW_LIMIT / threads;
    found = false;

    /* Configurar y lanzar hilos */
    for (j = 0; j < threads; j++) {
        arg[j].ini = n_intentos * j;
        arg[j].nintentos = (j != threads - 1) ? n_intentos : (POW_LIMIT - j * n_intentos);
        arg[j].found = &found;
        
        sem_wait(&data->memory);
        arg[j].objetivo = &data->actual.target;
        sem_post(&data->memory);
        
        arg[j].resultado = &resultado;

        if ((error = pthread_create(&hilos[j], NULL, miner, &arg[j]))) {
            fprintf(stderr, "pthread_create: %s\n", strerror(error));
            free(hilos);
            free(arg);
            return EXIT_FAILURE;
        }
    }

    /* Esperar a que terminen los hilos */
    for (j = 0; j < threads; j++) {
        if ((error = pthread_join(hilos[j], NULL))) {
            fprintf(stderr, "pthread_join: %s\n", strerror(error));
            free(hilos);
            free(arg);
            return EXIT_FAILURE;
        }
    }

    free(hilos);
    free(arg);

    /* Manejo del resultado */
    if (alarm_signal == 1) {
        terminar(data, queue, fd);
        return EXIT_SUCCESS;
    }
    else if (usr2_signal == 1) {
        perdedor(data);
    }
    else {
        if (sem_trywait(&data->ganador) == 0) {
            ganador(data, resultado, queue);
        } else {
            /* Espera activa mejorada para USR2 */
            
            while (timeout < 40) {  
                if (usr2_signal == 1) {
                    perdedor(data);
                    return EXIT_SUCCESS;
                }
                esperar_milisegundos(50);
                timeout++;
            }
            terminar(data, queue, fd);
            return EXIT_SUCCESS;
        }
    }

    nbytes= write(fd[1], (char*)&data->actual, sizeof(Bloque));

    if (nbytes == -1) {
        perror("Error al escribir en el pipe");
        terminar(data, queue, fd);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
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
    int timeout = 0;

    /*Registra el resultado y manda la señal a todos los procesos minero*/
    sem_wait(&data->memory);
    data->actual.result = resultado;
    data->actual.pid = getpid();

    for (i = 0, j=0; i < MAX_PIDS && j < (data->mineros - 1); i++) {
        if (data->pids[i] != 0 && data->pids[i] != getpid()) {
            kill(data->pids[i], SIGUSR2);
            j++;
        }
    }

    sem_post(&data->memory);

    sem_wait(&data->memory);

    data->votos[data->cont_votos] = (rand() % 2);
    data->cont_votos++;

    sem_post(&data->memory);

    while ( timeout < 20) {
        sem_post(&data->memory);

        if (data->cont_votos == (data->mineros)) {
            break;
        }
        sem_wait(&data->memory);
        esperar_milisegundos(100);
        timeout++;
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
    if (votos_favor *2 >= data->cont_votos) {
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
    sem_post(&data->iniciar);
    esperar_milisegundos(100);
    next_round(data);
}


void perdedor(Mem_Sys *data){
    

    sem_wait(&data->memory);

    data->votos[data->cont_votos] = (rand() % 2);
    data->cont_votos++;

    sem_post(&data->memory);

    usr2_signal = 0;
    usr1_signal = 0;

      while(usr1_signal == 0 && alarm_signal == 0) {
        esperar_milisegundos(100);
    }
}

void terminar(Mem_Sys *data, mqd_t queue, int *fd) {
    int status;
    abandonar_sistema(data);
    
    if (data->mineros == 0) {
        if (queue != (mqd_t)-1) {
            data->actual.finish = true;
            mq_send(queue, (char*)&data->actual, sizeof(Bloque), 0);
        }
        
        sem_destroy(&data->memory);
        sem_destroy(&data->ganador);
        sem_destroy(&data->iniciar);
    }

    while(data->mineros > 0) {
    }

    munmap(data, sizeof(Mem_Sys));
    mq_close(queue);

    close(fd[1]);
    wait(&status);
    fprintf(stdout,"Registrador terminó con estado %d\n", WEXITSTATUS(status));
    fflush(stdout);
    exit(EXIT_SUCCESS);
}


void abandonar_sistema(Mem_Sys *data) {
    int i;
    sem_wait(&data->iniciar);
    
    for ( i = 0; i < MAX_PIDS; i++) {
        if (data->pids[i] == getpid()) {
            data->pids[i] = 0;
            break;
        }
    }

    data->mineros--;
    
    if (data->mineros == 0) {
        data->actual.finish = true;
    }
    
    sem_post(&data->iniciar);
}