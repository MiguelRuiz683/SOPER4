/**
 * @file monitor.c
 * @author Rodrigo Verde y Miguel Ruiz
 * @brief Definición de la función monitor
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

#include "utilities.h"
#include "monitor1.h"

/**
 * Función monitor encargada de mostrar por pantalla los resultados del minero
 */
int monitor(int lag, int fd_shm) {
    data_message *data = NULL;
    bool flag=false;

    /**
     * Abre la memmoria compartida
     */
    data = mmap(NULL, sizeof(data_message), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (data == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    while (data->in != 0 || data->out != 0) {
    }
    
    while (1) {
        esperar_milisegundos(lag);

        /**
         * Con un sistema de semáforos productor distribuidor coge la información de la memoria
         * y la muestra por pantalla
         */
        sem_wait(&data->mutex);
        sem_wait(&data->mutex);

        if (data->in >= data->out) {
            if (data->correct[data->out%BUFFER_SIZE]) {
                fprintf(stdout, "Solution accepted: %08ld --> %08ld\n", data->target[data->out%BUFFER_SIZE], data->result[data->out%BUFFER_SIZE]);
            }else{
                printf( "Solution rejected: %08ld !-> %08ld\n", data->target[data->out%BUFFER_SIZE], data->result[data->out%BUFFER_SIZE]);
                fflush(stdout);
                flag=true;
            }
            if (data->finish[data->out%BUFFER_SIZE]) {
                flag=true;
            }
        }

        sem_post(&data->mutex);
        sem_post(&data->ganador);

        if (flag) {            
            munmap(data,sizeof(data_message));
            shm_unlink(SHM_NAME);
            sem_destroy(&data->ganador);
            sem_destroy(&data->mutex);
            sem_destroy(&data->mutex);
            return EXIT_SUCCESS;
        }
        
        data->out++;
    }
}