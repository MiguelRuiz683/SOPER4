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

#define BUFFER_SIZE 6
#define MAX_MSG 100
#define N_MSG 7
#define MAX_PIDS 100
#define MAX_VOT 100
#define SHM_NAME "/shm_data"
#define MQ_NAME "/mq_data"
 
 /**
  * @brief Función encargada de esperar ciertos milisegundos
  * 
  */
void esperar_milisegundos(int ms);


typedef struct {
  pid_t pid;          /*PID del minero*/ 
  int monedas;        /*Monedas del minero*/ 
} CarteraMinero;

typedef struct {
  int id_bloque;                      /*Id del bloque */
  long target;                        /*Número objetivo del minero*/
  long result;                        /*Número resultado del minero*/
  pid_t pid;                          /*PID del minero ganador*/ 
  int votos_tot;                      /*Votos totales*/
  int votos_pos;                      /*Votos positivos*/
  CarteraMinero carteras[MAX_PIDS];   /*Las carteras de los mineros que participaron en la ronda*/
} Bloque;


/**
 * Estructura de la escritura en memoria compartida
 */
typedef struct {
  pid_t pids[MAX_PIDS];               /*Número máximo de miembros en el sistema*/
  bool votos[MAX_PIDS];               /*Votos de cada minero*/
  int mineros;                        /*Número de mineros en el sistema*/
  int cont_votos;                     /*Contador de los votos registrados*/
  CarteraMinero carteras[MAX_PIDS];   /*Las carteras de los mienros activos*/
  Bloque ultimo;                      /*Último bloque resuelto*/
  Bloque actual;                      /*Bloque actual en resolución*/
  sem_t ganador;                      /*Semáforo de vacío de producto consumidor*/
  sem_t mutex;                    /*Semáforo de lleno de producto consumidor*/
  bool listo;                         /*Marca si el sistema está listo*/
  pid_t primero;                      /*Pid del priemr minero que participó en la minería*/
}Mem_Sys;

/**
 * Estructura de los mensajes por cola
 */
typedef struct {
  long target;    /*Número objetivo del minero*/
  long result;    /*Número resultado del minero*/
  bool finish;    /*Marca el final del proceso minero*/
} message;

 
 #endif