#include <stdio.h>
#include "bmat.h"

int main()
{
    Matrix A;

    initMatrixModule(521);
    initRandomModule();
    A = randomGoodMatrix();
    showMatrixInHex(A);
    return 0;
}
