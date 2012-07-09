#include <stdio.h>
#include <stdlib.h>
#include "bmat.h"
#include "generators.h"

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
    A = getGenerator(N);
    A = matrixPow(A, myPriv);
    sharedKey = matrixMultiplyVector(A, theirPub);
    showBignum(sharedKey);
    return 0;
}
