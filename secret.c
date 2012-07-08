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
    int N;

    myPriv = readKey(argv[1], true);
    theirPub = readKey(argv[2], false);
    if(myPriv == NULL || theirPub == NULL) {
        return 1;
    }
    N = getBignumSize(myPriv);
    if(N != getBignumSize(theirPub)) {
        printf("Keys are not the same size, and can not be used together.\n");
        return 1;
    }
    initMatrixModule(N);
    switch(N) {
    case 127: A = createMatrix(A127_data); break;
    case 521: A = createMatrix(A521_data); break;
    case 607: A = createMatrix(A607_data); break;
    default:
        printf("Unsupported key length %d\n", N);
        return 1;
    }
    A = matrixPow(A, myPriv);
    sharedKey = matrixMultiplyVector(A, theirPub);
    printf("Shared secret key between you (%s) and %s is\n", argv[1], argv[2]);
    showBignum(sharedKey);
    return 0;
}
