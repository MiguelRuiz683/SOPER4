/**
 * @file registrador.h
 * @author Rodrigo Verde y Miguel Ruiz
 * @brief Declaración de función registrador
 * 
 */

#ifndef _REGISTRADOR_H
#define _REGISTRADOR_H

#include "utilities.h"

/**
 * @brief Proceso registrador que se encarga de recibir los bloques y registrarlos en un fichero
 * 
 * @param fd Descriptor de archivo del pipe
 * @return int 
 */
int registrador(int *fd);

#endif