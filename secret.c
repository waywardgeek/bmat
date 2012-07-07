#include <stdio.h>
#include <stdlib.h>
#include "bmat.h"
#include "A127.h"
#include "A521.h"
#include "A607.h"

int main(int argc, char **argv)
{
    Matrix A;
    Bignum myPriv, theirPub, sharedKey;
    int N = 521;

    initMatrixModule(N);
    //A = createMatrix(A127_data);
    A = createMatrix(A521_data);
    myPriv = readKey(argv[1], N);
    theirPub = readKey(argv[2], N);
    A = matrixPow(A, myPriv);
    sharedKey = matrixMultiplyVector(A, theirPub);
    printf("Shared secret key between you (%s) and %s is\n", argv[1], argv[2]);
    showBignum(sharedKey);
    return 0;
}
