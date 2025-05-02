#include <stdio.h>
#include <stdlib.h>
#include "minero.h"

int main(int argc, char **argv) {
    int nrondas, lag;

    if (argc != 3) {
        fprintf(stderr, "Invalid input\n");
        exit(EXIT_FAILURE);
    } else if ((nrondas = atol(argv[1])) <= 0) {
        fprintf(stderr, "Invalid input in argument 1\n");
        exit(EXIT_FAILURE);
    } else if ((lag = atol(argv[2])) < 0) {
        fprintf(stderr, "Invalid input in argument 2\n");
        exit(EXIT_FAILURE);
    }

    minero(nrondas, lag);

    exit(EXIT_SUCCESS);
}