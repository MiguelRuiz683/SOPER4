/**
 * @file minero.h
 * @author Rodrigo Verde y Miguel Ruiz
 * @brief Declaración de función minero
 *
 */

#ifndef _MINERO_H
#define _MINERO_H

#define N_HILOS 20


/**
 * @brief Proceso minero se encarga de buscar la solución, y enviar los bloques al comprobador
 * @param nrondas número de rondas que hay que buscar una solución
 * @param espera tiempo de espera entre rondas
 */
int minero(int nrondas, int lag);

#endif