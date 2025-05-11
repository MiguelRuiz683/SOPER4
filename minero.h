/**
 * @file minero.h
 * @author Rodrigo Verde y Miguel Ruiz
 * @brief Declaración de función minero
 *
 */

#ifndef _MINERO_H
#define _MINERO_H


#include "utilities.h"
/**
 * @brief Proceso minero se encarga de buscar la solución, y enviar los bloques al comprobador
 * @param seconds tiempo que el minero estará activo
 * @param threads hilos que empleará para minar
 */
int minero(int seconds, int threads, Mem_Sys *data, int *fd);

#endif