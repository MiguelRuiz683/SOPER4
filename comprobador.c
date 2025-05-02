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

#include "comprobador.h"
#include "utilities.h"
#include "pow.h"

/**
 * Esta función es la encargada de comprobar si el resultado es el correcto y guardar en la memmoria los resultados
 */
void comprueba();

int comprobador(int lag, int fd_shm) {
    data_message *data = NULL;
    mqd_t queue;
    struct mq_attr attributes;
    unsigned int prior;
    message msg;

    attributes.mq_maxmsg = N_MSG;
    attributes.mq_msgsize = sizeof(message);

    data = mmap(NULL, sizeof(data_message), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (data == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

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
    
    data->in = 0;
    data->out = 0;
    
    queue = mq_open(MQ_NAME, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR, &attributes);

    while (1) {
        /*Recibe la información del minero*/
        if (mq_receive(queue, (char*)&msg, sizeof(message), &prior) == -1) {
            perror("mq_receive");
            exit(EXIT_FAILURE);
        }
        comprueba(msg, data);

        if (msg.finish==true) {
            munmap(data,sizeof(data_message));
            shm_unlink(SHM_NAME);
            mq_close(queue);
            break;
        }

        esperar_milisegundos(lag);
        
    }

    return EXIT_SUCCESS;
    
}


void comprueba(message msg, data_message *data){
    
    long objetivo = msg.target;
    long result = msg.result;

    sem_wait(&data->empty);
    sem_wait(&data->mutex);

    data->target[(data->in)%BUFFER_SIZE] = objetivo;
    data->result[(data->in)%BUFFER_SIZE] = result;

    /*Comprobación de la respuesta obtenida por el minero*/
    if (objetivo == pow_hash(result)) {
        data->correct[(data->in)%BUFFER_SIZE] = true;
        data->finish[(data->in)%BUFFER_SIZE] = false;
        if (msg.finish) {
            data->finish[(data->in)%BUFFER_SIZE] = true;
        }
    } else {
        data->correct[(data->in)%BUFFER_SIZE] = false;
        data->finish[(data->in)%BUFFER_SIZE] = true;
    }
    data->in+=1;

    sem_post(&data->mutex);
    sem_post(&data->fill);

    
    
}