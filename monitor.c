/**
 * @file monitor.c
 * @author Rodrigo Verde y Miguel Ruiz
 * @brief Definición de la función monitor
 *
 */
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>


#include "comprobador.h"
#include "monitor1.h"
#include "utilities.h"

#define INT_LIST_SIZE 10
#define MSG_MAX 100

/**
 * Main del monitor y el comprobador que crea la memoria compartida o la abre dependiendo de si
 * ya estaba creada o no
 * 
 */
int main(int argc, char **argv) {
  int lag, fd_shm;

  /*Control de errores*/
  if (argc != 2) {
    fprintf(stdout, "Invalid input\n");
    exit(EXIT_FAILURE);
  }

  if ((lag = atol(argv[1])) < 0) {
    fprintf(stdout, "Invalid input in argument 1\n");
    exit(EXIT_FAILURE);
  }


  /*Creación de la memoria y distribución de tareas*/
  fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  
  if (fd_shm == -1) {
    if (errno == EEXIST) {
      fd_shm = shm_open(SHM_NAME, O_RDWR, 0);
      if (fd_shm == -1) {
        perror("Error opening the shared memory segment");
        exit(EXIT_FAILURE);
      }
      else {
        fprintf(stdout, "[%d] Printing bloks...\n", getpid());
        fflush(stdout);
        monitor(lag, fd_shm);
        fprintf(stdout, "[%d] Finishing...\n", getpid());
        fflush(stdout);
      }
    }
    else {
      perror("Error creating the shared memory segment\n");
      exit(EXIT_FAILURE);
    }
  }
  else {
    if (ftruncate(fd_shm, sizeof(data_message)) == -1) {
      perror("ftruncate");
      shm_unlink(SHM_NAME);
      exit(EXIT_FAILURE);
    }
    fprintf(stdout, "[%d] Checking blocks...\n", getpid());
    fflush(stdout);
    comprobador(lag, fd_shm);
    fprintf(stdout, "[%d] Finishing...\n", getpid());
    fflush(stdout);
  }


  exit(EXIT_SUCCESS);
}