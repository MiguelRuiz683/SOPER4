/**
 * @file wait.h
 * @author Rodrigo Verde y Miguel Ruiz
 * @brief Declaración de la función espera
 *
 */

 #ifndef _UTILITIES_H
 #define _UTILITIES_H

#include <stdbool.h>
#include <semaphore.h>
#include "pow.h"

#define BUFFER_SIZE 5
#define MAX_MSG 100
#define N_MSG 7
#define MAX_PIDS 100
#define MAX_VOT 100
#define SHM_NAME "/shm_data"
#define SHM_NAME2 "/shm_monitor"
#define MQ_NAME "/mq_data"
 
 /**
  * @brief Función encargada de esperar ciertos milisegundos
  * 
  * @param el tiempo en ms que se quiere esperar
  */
void esperar_milisegundos(int ms);


typedef struct {
  pid_t pid;                          /*PID del minero*/ 
  int monedas;                        /*Monedas del minero*/ 
} CarteraMinero;

typedef struct {
  int id_bloque;                      /*Id del bloque */
  long target;                        /*Número objetivo del minero*/
  long result;                        /*Número resultado del minero*/
  pid_t pid;                          /*PID del minero ganador*/ 
  int votos_tot;                      /*Votos totales*/
  int votos_pos;                      /*Votos positivos*/
  CarteraMinero carteras[MAX_PIDS];   /*Las carteras de los mineros que participaron en la ronda*/
  bool finish;                        /*Marca si el bloque es el último*/
} Bloque;


/**
 * Estructura de la escritura en memoria compartida
 */
typedef struct {
  pid_t pids[MAX_PIDS];               /*Alamcena todos los pids del sistema*/
  bool votos[MAX_PIDS];               /*Votos de cada minero*/
  int mineros;                        /*Número de mineros en el sistema*/
  int cont_votos;                     /*Contador de los votos registrados*/
  CarteraMinero carteras[MAX_PIDS];   /*Las carteras de los mienros activos*/
  Bloque ultimo;                      /*Último bloque resuelto*/
  Bloque actual;                      /*Bloque actual en resolución*/
  sem_t ganador;                      /*Semáforo que impide a más de dos procesos ser el ganador*/
  sem_t memory;                       /*Semáforo que impide a más de dos procesos acceder a memoria*/
  sem_t iniciar;                      /*Semáforo que impide a un proceso registrarse mientras se está minando*/
  bool listo;                         /*Marca si el sistema está listo para iniciarse*/
  pid_t primero;                      /*Pid del priemr minero que participó en la minería*/
} Mem_Sys;

typedef struct {
  Bloque bloques[BUFFER_SIZE];        /*Bloques que se han resuelto*/
  bool correct[BUFFER_SIZE];          /*Boolenao que marca si el resultado es correcto*/
  sem_t empty;                        /*Semáforo de vacío de producto consumidor*/
  sem_t fill;                         /*1Semáforo de lleno de producto consumidor*/
  sem_t mutex;                        /*Semáforo de mutex de producto consumidor*/
  int in;                             /*Contador de los datos que se guardan*/
  int out;                            /*Contador de los datos que se sacan*/
  bool finish[BUFFER_SIZE];           /*Marca si se debe terminar por motivos externos*/
} data_message;

 
 #endif