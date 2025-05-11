/**
 * @file registrador.h
 * @author Rodrigo Verde y Miguel Ruiz
 * @brief Definición de función registrador
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "registrador.h"

 int registrador(int *fd) {
    int file;
    char filename[MAX_MSG];
    pid_t pid;
    Bloque bloque;
    int nbytes = 1;
    int i;

    pid = getpid();

    snprintf(filename, sizeof(filename), "Data_%d.log", pid);

    file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (file == -1) {
        close(fd[0]);
        return EXIT_FAILURE;
    }

    while (nbytes > 0) {
        nbytes = read(fd[0], (char*)&bloque, sizeof(Bloque));
        if (nbytes == -1) {
            close(fd[0]);
            close(file);
            EXIT_FAILURE;
        }

        dprintf(file, "Id: %d\n", bloque.id_bloque);
        dprintf(file, "Winner: %d\n", bloque.pid);
        dprintf(file, "Target: %ld\n", bloque.target);
        if (bloque.votos_pos >= bloque.votos_tot/2) {
            dprintf(file, "Solution: %ld (validated)\n", bloque.result);
        } else {
            dprintf(file, "Solution: %ld (rejected)\n", bloque.result);
        }
        dprintf(file, "Votes: %d/%d\n", bloque.votos_pos, bloque.votos_tot);
        dprintf(file, "Wallets:");

        for (i = 0; i < bloque.votos_tot; i++) {
            dprintf(file, " %d:%d", bloque.carteras[i].pid, bloque.carteras[i].monedas);
        }
        dprintf(file, "\n\n");
        fflush(stdout);
    }

    return EXIT_SUCCESS;
 }