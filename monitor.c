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
#include <mqueue.h>


#include "comprobador.h"
#include "monitor1.h"
#include "utilities.h"

#define INT_LIST_SIZE 10
#define MSG_MAX 100

/**
 * Main del monitor y el comprobador
 * 
 */
int main(int argc, char **argv) {
  int fd_shm;
  pid_t monitor_pid;

  /*Control de errores*/
  if (argc != 1) {
    fprintf(stdout, "Invalid input\n");
    exit(EXIT_FAILURE);
  }


  shm_unlink(SHM_NAME2);
  mq_unlink(MQ_NAME);
  
  fd_shm = shm_open(SHM_NAME2, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (fd_shm == -1) {
    perror("Error al crear el fichero de memoria compartida");
    exit(EXIT_FAILURE);
  }

  monitor_pid = fork();

  if (monitor_pid == -1) {
    perror("Error al crear el proceso hijo");
    exit(EXIT_FAILURE);
  } else if (monitor_pid == 0) {
    if (ftruncate(fd_shm, sizeof(data_message)) == -1) {
      perror("ftruncate");
      shm_unlink(SHM_NAME);
      exit(EXIT_FAILURE);
    }
    comprobador(fd_shm);
  } else {
    monitor(fd_shm);
  }

  exit(EXIT_SUCCESS);
}