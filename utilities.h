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

#define BUFFER_SIZE 6
#define MAX_MSG 100
#define N_MSG 7
#define SHM_NAME "/shm_data"
#define MQ_NAME "/mq_data"
 
 /**
  * @brief Función encargada de esperar ciertos milisegundos
  * 
  */
void esperar_milisegundos(int ms);

/**
 * Estructura de la escritura en memoria compartida
 */
typedef struct {
  long target[BUFFER_SIZE];  /*Número objetivo del minero*/
  long result[BUFFER_SIZE];   /*Número resultado del minero*/
  bool correct[BUFFER_SIZE];  /*Boolenao que marca si el resultado es correcto*/
  bool finish[BUFFER_SIZE];   /*Booleano que marca si es el último resultado*/
  sem_t empty;                /*Semáforo de vacío de producto consumidor*/
  sem_t fill;                 /*Semáforo de lleno de producto consumidor*/
  sem_t mutex;                /*Semáforo de mutex de producto consumidor*/
  int in;                     /*Contador de los datos que se guardan*/
  int out;                    /*Contador de los datos que se sacan*/
}data_message;

/**
 * Estructura de los mensajes por cola
 */
typedef struct {
  long target;    /*Número objetivo del minero*/
  long result;    /*Número resultado del minero*/
  bool finish;    /*Marca el final del proceso minero*/
} message;

 
 #endif