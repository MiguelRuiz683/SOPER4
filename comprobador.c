/**
 * @file comprobador.c
 * @author Rodrigo Verde y Miguel Ruiz
 * @brief Declaración de función comprobador
 *
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <mqueue.h>
#include <signal.h>


#include "comprobador.h"
#include "utilities.h"
#include "pow.h"


static volatile sig_atomic_t int_signal = 0;

void handler(int sig) {
    if (sig == SIGINT) {
        int_signal = 1;
    }
}

/**
 * Esta función es la encargada de comprobar si el resultado es el correcto y guardar en la memmoria los resultados
 */
void comprueba();

int comprobador(int fd_shm) {
    data_message *data = NULL;
    mqd_t queue;
    struct mq_attr attributes;
    unsigned int prior;
    Bloque bloque;
    bool finish = false;

    attributes.mq_maxmsg = N_MSG;
    attributes.mq_msgsize = sizeof(Bloque);
    queue = mq_open(MQ_NAME, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR, &attributes);

    
    data = mmap(NULL, sizeof(data_message), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (data == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    data->in = 0;
    data->out = 0;

    /*Inicialización de semáforos con control de errores*/
    if (sem_init(&data->empty, 1, MAX_MSG) == -1) {
        perror("sem_init empty");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&data->fill, 1, 0) == -1) {
        perror("sem_init full");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&data->mutex, 1, 1) == -1) {
        perror("sem_init mutex");
        exit(EXIT_FAILURE);
    }
    
    

    while (1) {
        /*Recibe la información del minero*/

        if (mq_receive(queue, (char*) &bloque, sizeof(Bloque), &prior) == -1) {
            perror("mq_receive");
            exit(EXIT_FAILURE);
        }

        if (finish == false) {
            comprueba(bloque, data, &finish);
        }

        if (bloque.finish == true) {
            munmap(data,sizeof(data_message));
            mq_close(queue);
            break;
        }       
    }

    return EXIT_SUCCESS;
}


void comprueba(Bloque bloque, data_message *data, bool *finish){

    sem_wait(&data->empty);
    sem_wait(&data->mutex);

    if (int_signal == 1) {
        data->finish[data->in%BUFFER_SIZE] = true;
        *finish = true;
    }

    /*Comprobación de la respuesta obtenida por el minero*/
    if (bloque.target == pow_hash(bloque.result)) {
        data->correct[(data->in)%BUFFER_SIZE] = true;
    } else {
        data->correct[(data->in)%BUFFER_SIZE] = false;
    }
    data->bloques[(data->in)%BUFFER_SIZE] = bloque;
    data->in++;

    sem_post(&data->mutex);
    sem_post(&data->fill);
}