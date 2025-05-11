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
int monitor(int fd_shm) {
    data_message *data = NULL;
    bool flag=false;
    int i;

    /**
     * Abre la memmoria compartida
     */
    data = mmap(NULL, sizeof(data_message), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
    close(fd_shm);
    if (data == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    esperar_milisegundos(100);
    while (data->in == 0 ) {
        esperar_milisegundos(100);
    }

    
    while (1) {
        /**
         * Con un sistema de semáforos productor distribuidor coge la información de la memoria
         * y la muestra por pantalla
         */
        sem_wait(&data->fill);
        sem_wait(&data->mutex);


        if (data->in > data->out) {
            if (data->bloques[data->out%BUFFER_SIZE].finish == true || data->finish[data->out%BUFFER_SIZE] == true) {
                flag = true;
            } else{ 
                fprintf(stdout, "Id: %d\n", data->bloques[data->out%BUFFER_SIZE].id_bloque);
                fprintf(stdout, "Winner: %d\n", data->bloques[data->out%BUFFER_SIZE].pid);
                fprintf(stdout, "Target: %ld\n", data->bloques[data->out%BUFFER_SIZE].target);
                if (data->correct[data->out%BUFFER_SIZE] == true) {
                    fprintf(stdout, "Solution: %ld (validated)\n", data->bloques[data->out%BUFFER_SIZE].result);
                } else {
                    fprintf(stdout, "Solution: %ld (rejected)\n", data->bloques[data->out%BUFFER_SIZE].result);
                }
                fprintf(stdout, "Votes: %d/%d\n", data->bloques[data->out%BUFFER_SIZE].votos_pos, data->bloques[data->out%BUFFER_SIZE].votos_tot);
                fprintf(stdout, "Wallets:");

                for ( i = 0; i < data->bloques[data->out%BUFFER_SIZE].votos_tot; i++) {
                    fprintf(stdout, " %d:%d", data->bloques[data->out%BUFFER_SIZE].carteras[i].pid, data->bloques[data->out%BUFFER_SIZE].carteras[i].monedas);
                }
                printf("\n\n");
                fflush(stdout);
            }
        }

        data->out++;

        sem_post(&data->mutex);
        sem_post(&data->empty);

        if (flag) {            
            munmap(data, sizeof(data_message));
            close(fd_shm);
            shm_unlink(SHM_NAME2);
            shm_unlink(SHM_NAME);
            sem_destroy(&data->fill);
            sem_destroy(&data->empty);
            sem_destroy(&data->mutex);
            return EXIT_SUCCESS;
        }
    }
}