#include <stdio.h>
#include "bmat.h"
#include "A127.h"

int main(int argc, char **argv)
{
    Matrix A;
    Bignum myPriv, theirPub, sharedKey;

    initMatrixModule(127);
    A = createMatrix(A127_data);
    myPriv = readKey(argv[1], 127);
    theirPub = readKey(argv[2], 127);
    A = matrixPow(A, myPriv);
    sharedKey = matrixMultiplyVector(A, theirPub);
    printf("Shared secret key between you (%s) and %s is\n", argv[1], argv[2]);
    showBignum(sharedKey);
    return 0;
}
