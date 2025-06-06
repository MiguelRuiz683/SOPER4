/**
 * @file monitor.h
 * @author Rodrigo Verde y Miguel Ruiz
 * @brief Declaración de la función monitor
 *
 */

#ifndef _MONITOR_H
#define _MONITOR_H

/**
 * @brief Función encargada de la comprobación del resultado obtenido por minero
 * 
 * @param fd_shm descriptor de la memoria de datos compartida
 */
int monitor(int fd_shm);

#endif