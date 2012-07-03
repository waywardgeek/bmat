#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// If setting N to 64, also change the dotProd function.
#define WIDTH 32
static int N = WIDTH;

// This table is for computing the parity of bits of 16-bit ints.
static unsigned char parityTable[1 << 16];

typedef unsigned long Row;

typedef struct MatrixStruct *Matrix;

struct MatrixStruct {
    Row rowVal[WIDTH];
    Matrix nextFreeMatrix;
};

static Matrix firstFreeMatrix = NULL; // I'll maintain a free list of matricies.

static inline void setBit(Matrix M, int row, int col, int value)
{
    if(value) {
       M->rowVal[row] |= 1 << col;
    } else {
       M->rowVal[row] &= 1 << col;
    }
}

static inline int getBit(Matrix M, int row, int col)
{
    return (M->rowVal[row] >> col) & 1;
}

static Matrix zero(void)
{
    Matrix M = firstFreeMatrix;

    if(M != NULL) {
        firstFreeMatrix = M->nextFreeMatrix;
        memset((void *)M, 0, sizeof(struct MatrixStruct));
    } else {
        M = (Matrix)calloc(1, sizeof(struct MatrixStruct));
    }
    return M;
}

static Matrix identity(void)
{
    Matrix M = zero();
    int pos;

    for(pos = 0; pos < N; pos++) {
       setBit(M, pos, pos, 1);
    }
    return M;
}

static inline void del(Matrix M)
{
    M->nextFreeMatrix = firstFreeMatrix;
    firstFreeMatrix = M;
}

static void show(Matrix M, char *title)
{
    int row, col;

    if(title != NULL && *title != '\0') {
        printf("%s\n", title);
    }
    for(row = 0; row < N; row++) {
        printf("%d", getBit(M, row, 0));
        for(col = 1; col < N; col++) {
            printf(" %d", getBit(M, row, col));
        }
        printf("\n");
    }
    printf("\n");
}

static Matrix rotate(Matrix M)
{
    Matrix res = zero();
    int row, col;

    for(row = 0; row < N; row++) {
        for(col = 0; col < N; col++) {
            setBit(res, row, col, getBit(M, col, N - row - 1));
        }
    }
    return res;
}

// Computes one value in matrix multiply, but N must be rotated.
static int dotProd(Matrix A, Matrix B, int row, int col)
{
    Row vect = A->rowVal[row] & B->rowVal[col];

    // Note: this must be changed if working with 64-bit!
    return parityTable[vect >> 16] ^ parityTable[vect & 0xffff];
}

// This assumes B has been rotated, and is faster.
static Matrix multiplyRotated(Matrix A, Matrix B)
{
    Matrix res = zero();
    int row, col;

    for(row = 0; row < N; row++) {
        for(col = 0; col < N; col++) {
            setBit(res, row, col, dotProd(A, B, row, col));
        }
    }
    return res;
}

// This is much slower, since it has to rotate N first.
static Matrix multiply(Matrix A, Matrix B)
{
    Matrix Brot = rotate(B);
    Matrix res = multiplyRotated(A, Brot);

    del(Brot);
    return res;
}

static unsigned char xorSum(int n)
{
    unsigned char value = 0;

    while(n != 0) {
        if(n & 1) {
            value ^= 1;
        }
        n >>= 1;
    }
    return value;
}

static void initParityTable(void)
{
    int i;

    for(i = 0; i < (1 << 16); i++) {
        parityTable[i] = xorSum(i);
    }
}

int main()
{
    initParityTable();
    return 0;
}
