#include <stdio.h>
#include <stdlib.h>
#include "bmat.h"

int main(int argc, char **argv)
{
    Matrix G;
    int N;

    if(argc != 2) {
        printf("Usage: genmatrix size\n");
        return 1;
    }
    N = atoi(argv[1]);
    if(N < 2) {
        printf("size must be >= 2\n");
        return 1;
    }
    initMatrixModule(N);
    initRandomModule(true);
    G = randomGoodMatrix();
    showMatrixInHex(G);
    return 0;
}
