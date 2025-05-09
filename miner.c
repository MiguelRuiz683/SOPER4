#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>       
#include <string.h>




#include "minero.h"
#include "utilities.h"


void valores_defecto(Mem_Sys *data);

int main(int argc, char **argv) {
    int n_seconds, n_threads;
    int fd_shm;
    int i;
    Mem_Sys *data = NULL;
    int monitor;
    if (argc != 3) {
        fprintf(stderr, "Invalid input\n");
        exit(EXIT_FAILURE);
    } else if ((n_seconds = atol(argv[1])) <= 0) {
        fprintf(stderr, "Invalid input in argument 1\n");
        exit(EXIT_FAILURE);
    } else if ((n_threads = atol(argv[2])) < 1) {
        fprintf(stderr, "Invalid input in argument 2\n");
        exit(EXIT_FAILURE);
    }

    esperar_milisegundos(100);
    monitor = shm_open(SHM_NAME2, O_RDWR, 0);
    if (monitor == -1) {
        perror("shm_open");
        if (errno == ENOENT) {
            fprintf(stderr, "El monitor no se ha iniciado\n");
        }
        exit(1);
    }
    close(monitor);
    fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  

    if (fd_shm == -1) {
      if (errno == EEXIST) {
        fd_shm = shm_open(SHM_NAME, O_RDWR, 0);
        if (fd_shm == -1) {
          perror("Error opening the shared memory segment");
          exit(EXIT_FAILURE);
        }
        else {
        
          data = mmap(NULL, sizeof(Mem_Sys), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
          close(fd_shm);
          while (!data->listo) {
              esperar_milisegundos(100);
          }
        }
      }
      else {
        perror("Error creating the shared memory segment\n");
        exit(EXIT_FAILURE);
      }
    }
    else {
        
      if (ftruncate(fd_shm, sizeof(Mem_Sys)) == -1) {
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
      }
  
      data = mmap(NULL, sizeof(Mem_Sys), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
      close(fd_shm);
      if (data == MAP_FAILED) {
          perror("mmap");
          shm_unlink(SHM_NAME);
          exit(EXIT_FAILURE);
      }
      memset(data, 0, sizeof(Mem_Sys));
      valores_defecto(data);

    }
    sem_wait(&data->memory);
    if (data->mineros == MAX_PIDS) {
        printf("El sistema está lleno");
        exit(EXIT_SUCCESS);
    }
    for ( i = 0; i <= data->mineros; i++) {
        if (data->pids[i] == 0) {
            data->pids[i] = getpid();
            break;
        }
    }
    for ( i = 0; i <= data->mineros; i++) {
        if (data->carteras[i].pid == 0) {
            data->carteras[i].pid = getpid();
        }
        
    }
    data->mineros++;
    sem_post(&data->memory);
    
    
    minero(n_seconds, n_threads, data);


    exit(EXIT_SUCCESS);
}



void valores_defecto(Mem_Sys *data){
    data->listo = false;
    data->mineros = 0;

    data->actual.target = 0;
    
    data->primero = getpid();

    data->actual.votos_tot = 0;

    if (sem_init(&data->ganador, 1, 1) == -1) {
        perror("sem_init ganador");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&data->memory, 1, 1) == -1) {
        perror("sem_init full");
        exit(EXIT_FAILURE);
    }
    data->listo = true;

}