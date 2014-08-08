#include <stdio.h>
#include <stdlib.h>
#include "bmat.h"

// Just check the theory that our method of choosing good generator matrices
// works, by brute-force computing their order.
int main(int argc, char **argv)
{
    //int N;

    initRandomModule(true);
    //initMatrixModule(2);
    //for(N = 2; N < 40; N++) {
    initMatrixModule(61);
    while(true) {
        //initMatrixModule(N);
        checkPrimeOrderTheory();
    }
    return 0;
}
