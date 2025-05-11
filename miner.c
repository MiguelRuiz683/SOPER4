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
#include <sys/types.h>
#include <sys/wait.h>



#include "minero.h"
#include "utilities.h"
#include "registrador.h"


void valores_defecto(Mem_Sys *data);
int registrar_minero(Mem_Sys *data);

int main(int argc, char **argv) {
    int n_seconds, n_threads;
    int fd_shm;
    Mem_Sys *data = NULL;
    int monitor;
    int fd[2];
    int pipe_status;
    pid_t registrador_pid;
    int status1, status2;

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

    if (fd_shm == -1 && errno == EEXIST) {
        fd_shm = shm_open(SHM_NAME, O_RDWR, 0);
        if (fd_shm == -1) {
            perror("Error al abrir memoria compartida");
            exit(EXIT_FAILURE);
        }
        
        data = mmap(NULL, sizeof(Mem_Sys), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
        close(fd_shm);
        
        if (data == MAP_FAILED) {
            perror("Error en mmap");
            exit(EXIT_FAILURE);
        }

        while (!data->listo) {
            esperar_milisegundos(100);
        }
    } 
    else if (fd_shm == -1) {
        perror("Error al crear memoria compartida");
        exit(EXIT_FAILURE);
    }
    else {
        if (ftruncate(fd_shm, sizeof(Mem_Sys)) == -1) {
            perror("Error en ftruncate");
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }
  
        data = mmap(NULL, sizeof(Mem_Sys), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
        close(fd_shm);
        
        if (data == MAP_FAILED) {
            perror("Error en mmap");
            shm_unlink(SHM_NAME);
            exit(EXIT_FAILURE);
        }
        
        memset(data, 0, sizeof(Mem_Sys));
        valores_defecto(data);
    }

    if (!registrar_minero(data)) {
        munmap(data, sizeof(Mem_Sys));
        fprintf(stderr, "El sistema está lleno. Inténtelo más tarde.\n");
        exit(EXIT_FAILURE);
    }

    pipe_status = pipe(fd);
    if (pipe_status == -1) {
        munmap(data, sizeof(Mem_Sys));
        if (data->mineros == 0) {
            shm_unlink(SHM_NAME);
        }
        perror("Pipe");
        exit(EXIT_FAILURE);
    }

    registrador_pid = fork();
    if (registrador_pid == -1) {
        munmap(data, sizeof(Mem_Sys));
        if (data->mineros == 0) {
            shm_unlink(SHM_NAME);
        }
        perror("Fork");
        exit(EXIT_FAILURE);
    } else if (registrador_pid == 0) {
        munmap(data, sizeof(Mem_Sys));
        close(fd[1]);
        if (registrador(fd) == EXIT_FAILURE) {
            exit(EXIT_FAILURE);
        }
    } else {
        close(fd[0]);
        status1 = minero(n_seconds, n_threads, data, fd);
        close(fd[1]);
        wait(&status2);
        fprintf(stdout,"Registrador terminó con estado %d\n", WEXITSTATUS(status2));
        if (status1 == EXIT_FAILURE) {
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}

void valores_defecto(Mem_Sys *data){
    data->listo = false;
    data->mineros = 0;

    data->actual.target = 0;
    
    data->primero = getpid();

    data->actual.votos_tot = 0;

    if (sem_init(&data->ganador, 1, 1) == -1) {
        munmap(data, sizeof(Mem_Sys));
        shm_unlink(SHM_NAME);
        perror("sem_init ganador");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&data->memory, 1, 1) == -1) {
        munmap(data, sizeof(Mem_Sys));
        shm_unlink(SHM_NAME);
        perror("sem_init memory");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&data->iniciar, 1, 1) == -1) {
        munmap(data, sizeof(Mem_Sys));
        shm_unlink(SHM_NAME);
        perror("sem_init iniciar");
        exit(EXIT_FAILURE);
    }
    data->listo = true;
}

int registrar_minero(Mem_Sys *data) {
    int slot_libre = -1;
    int cartera_registrada = 0;
    int i;

    sem_wait(&data->iniciar);

    for ( i = 0; i < MAX_PIDS; i++) {
        if (data->pids[i] == 0) {
            slot_libre = i;
            break;
        }
    }

    if (slot_libre == -1) {
        sem_post(&data->iniciar);
        return 0; 
    }

    data->pids[slot_libre] = getpid();
    
    for ( i = 0; i < MAX_PIDS; i++) {
        if (data->carteras[i].pid == getpid()) {
            cartera_registrada = 1;
            break;
        }
    }
    
    if (!cartera_registrada) {
        for ( i = 0; i < MAX_PIDS; i++) {
            if (data->carteras[i].pid == 0) {
                data->carteras[i].pid = getpid();
                data->carteras[i].monedas = 0;
                break;
            }
        }
    }

    data->mineros++;
    
    sem_post(&data->iniciar);
    return 1;
}