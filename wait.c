/**
 * @file wait.c
 * @author Rodrigo Verde y Miguel Ruiz
 * @brief Definición de la función esperar_milisegundos
 *
 */

#define _POSIX_C_SOURCE      199309L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include "utilities.h"

void esperar_milisegundos(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000; 
    nanosleep(&ts, NULL);
}