#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "bmat.h"

static int N; // Width of matrices.
static int numWords; // How many uint64 words are in each row.

static Matrix firstFreeMatrix = NULL; // I'll maintain a free list of matricies.

// This table is for computing the parity of bits of 16-bit ints.
byte parityTable[1 << 16];

// This is a queue of temporary matrices, which get reused after a while.  To
// allocate a matrix permanently, call copy.
#define QUEUE_LEN 256
static Matrix matrixQueue[QUEUE_LEN];
static int queuePos = 0;

// Structure definitions
struct MatrixStruct {
    Matrix nextMatrix;
    uint64 power; // Written to matrices in hash table.
    uint64 data[1]; // Accessed by row*numWords + col/64
};

struct HashTableStruct {
    unsigned size;
    Matrix *matrices;
};

static void setWidth(int width)
{
    N = width;
    numWords = (N + 63)/64;
}

// Hash table functions.

static inline unsigned hashValues(unsigned hash1, unsigned hash2)
{
    return hash1 ^ (hash2*1103515245 + 12345);
}

static unsigned hashMatrix(Matrix M)
{
    int i;
    unsigned int hash = 0;
    uint64 *p = M->data;

    for(i = N*numWords; i != 0; i--) {
        hash = hashValues(hash, (unsigned)*p);
        hash = hashValues(hash, (unsigned)(*p++ >> 32));
    }
    return hash;
}

static HashTable createHashTable(unsigned size)
{
    HashTable table = (HashTable)calloc(1, sizeof(struct HashTableStruct));

    table->size = size;
    table->matrices = (Matrix *)calloc(size, sizeof(Matrix));
    return table;
}

static bool equal(Matrix A, Matrix B)
{
    int i;
    uint64 *p = A->data;
    uint64 *q = B->data;

    for(i = N*numWords; i != 0; i--) {
        if(*p++ != *q++) {
            return false;
        }
    }
    return true;
}

static Matrix lookupInHashTable(HashTable hashTable, Matrix M)
{
    Matrix otherM = hashTable->matrices[hashMatrix(M) % hashTable->size];

    while(otherM != NULL && !equal(M, otherM)) {
        otherM = otherM->nextMatrix; // We reuse this field for the hash table chain
    }
    return otherM;
}

Matrix allocateMatrix(Matrix oldM)
{
    Matrix M = firstFreeMatrix;
    static int totalAllocated = 0;
    size_t size = sizeof(struct MatrixStruct) + (N*numWords - 1)*sizeof(uint64);

    if(M != NULL) {
        firstFreeMatrix = M->nextMatrix;
        M->nextMatrix = NULL;
    } else {
        M = (Matrix)malloc(size);
        totalAllocated++;
        //if((totalAllocated % 1000) == 0) {
            //printf("Allocated %d matrices\n", totalAllocated);
        //}
    }
    if(oldM != NULL) {
        memcpy((void *)M, (void *)oldM, size);
    }
    M->nextMatrix = NULL;
    return M;
}

static void addToHashTable(HashTable hashTable, Matrix M)
{
    unsigned index = hashMatrix(M) % hashTable->size;
    Matrix otherM = hashTable->matrices[index];

    M = allocateMatrix(M);
    M->nextMatrix = otherM;
    hashTable->matrices[index] = M;
}

void deleteMatrix(Matrix M)
{
    M->nextMatrix = firstFreeMatrix;
    firstFreeMatrix = M;
}

static void delHashTable(HashTable hashTable)
{
    Matrix M, nextM;
    int i;

    for(i = 0; i < hashTable->size; i++) {
        M = hashTable->matrices[i];
        while(M != NULL) {
            nextM = M->nextMatrix;
            deleteMatrix(M);
            M = nextM;
        }
    }
    free(hashTable->matrices);
    free(hashTable);
}

static inline Matrix newMatrix(void)
{
    if(++queuePos == QUEUE_LEN) {
        queuePos = 0;
    }
    return matrixQueue[queuePos];
}

static inline Matrix copy(Matrix oldM)
{
    Matrix M = newMatrix();

    memcpy((void *)M, (void *)oldM,
        sizeof(struct MatrixStruct) + (N*numWords - 1)*sizeof(uint64));
    M->nextMatrix = NULL;
    return M;
}

static inline void setBit(Matrix M, int row, int col, int value)
{
    int word = col >> 6;
    int bit = col & 0x3f;

    if(value) {
        M->data[row*numWords + word] |= 1LL << bit;
    } else {
        M->data[row*numWords + word] &= ~(1LL << bit);
    }
}

static inline int getBit(Matrix M, int row, int col)
{
    int word = col >> 6;
    int bit = col & 0x3f;

    return (M->data[row*numWords + word] >> bit) & 1;
}

static void setRow(Matrix A, int row, Bignum n)
{
    memcpy(A->data + row*numWords, getBignumData(n), numWords*sizeof(uint64));
}

static Bignum getRow(Matrix A, int row)
{
    Bignum n = createBignum(0, N);

    memcpy(getBignumData(n), A->data + row*numWords, numWords*sizeof(uint64));
    return n;
}

Bignum getMatrixColumn(Matrix A, int column)
{
    Bignum n = createBignum(0, N);
    int i;

    for(i = 0; i < N; i++) {
        setBignumBit(n, i, getBit(A, i, column));
    }
    return n;
}

static Matrix zero(void)
{
    Matrix M = newMatrix();

    memset((void *)M, 0, sizeof(struct MatrixStruct) + (N*numWords - 1)*sizeof(uint64));
    return M;
}

Matrix createMatrix(uint64 *data)
{
    Matrix M = zero();

    memcpy(M->data, data, N*numWords*sizeof(uint64));
    return allocateMatrix(M);
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

void showMatrix(Matrix M)
{
    int row, col;

    for(row = 0; row < N; row++) {
        for(col = 0; col < N; col++) {
            printf("%d", getBit(M, row, col));
        }
        printf("\n");
    }
    printf("\n");
}

void showMatrixInHex(Matrix M)
{
    int row, word;
    uint64 *p = M->data;

    printf("uint64 G%d_data[%d] = {\n", N, N*numWords);
    for(row = 0; row < N; row++) {
        for(word = 0; word < numWords; word++) {
            if(word != 0) {
                printf(", ");
            }
            printf("0x%llxLL", *p++);
        }
        printf(",\n");
    }
    printf("};\n");
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

static Matrix transpose(Matrix M)
{
    Matrix res = zero();
    int row, col;

    for(row = 0; row < N; row++) {
        for(col = 0; col < N; col++) {
            setBit(res, row, col, getBit(M, col, row));
        }
    }
    return res;
}


static Matrix add(Matrix A, Matrix B)
{
    Matrix res = zero();
    int row, col;

    for(row = 0; row < N; row++) {
        for(col = 0; col < N; col++) {
            setBit(res, row, col, getBit(A, row, col) ^ getBit(B, row, col));
        }
    }
    return res;
}

// Computes one value in matrix multiply, but N must be transposed.
static inline int dotProd(Matrix A, Matrix B, int row, int col)
{
    uint64 *p = A->data + row*numWords;
    uint64 *q = B->data + col*numWords;
    uint64 v;
    int value = 0;
    int i;

   
    for(i = 0; i < numWords; i++) {
        v = *p++ & *q++;
        value ^= parityTable[(unsigned short)v] ^ parityTable[(unsigned short)(v >> 16)] ^
            parityTable[(unsigned short)(v >> 32)] ^ parityTable[(unsigned short)(v >> 48)];
    }
    return value;
}

static inline int dotProdVect(Matrix A, Bignum n, int row)
{
    uint64 *p = A->data + row*numWords;
    uint64 word;
    uint64 v;
    int value = 0;
    int i;

   
    for(i = 0; i < numWords; i++) {
        word = getBignumWord(n, i);
        v = *p++ & word;
        value ^= parityTable[(unsigned short)v] ^ parityTable[(unsigned short)(v >> 16)] ^
            parityTable[(unsigned short)(v >> 32)] ^ parityTable[(unsigned short)(v >> 48)];
    }
    return value;
}

// This assumes B has been transposed, and is faster.
static Matrix multiplyTransposed(Matrix A, Matrix B)
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

// This is slower, since it has to transpose N first.
Matrix matrixMultiplySlow(Matrix A, Matrix B)
{
    return multiplyTransposed(A, transpose(B));
}

// XOR the source row into the dest row.
static inline void xorMatrixRows(Matrix S, Matrix D, int source, int dest)
{
    uint64 *s = S->data + source*numWords;
    uint64 *d = D->data + dest*numWords;
    int i;

    for(i = 0; i < numWords; i++) {
        *d++ ^= *s++;
    }
}

// Another, hopefully faster multiply.
Matrix matrixMultiply(Matrix A, Matrix B)
{
    Matrix res = zero();
    uint64 word;
    int row, xWord, bit;

    for(row = 0; row < N; row++) {
        for(xWord = 0; xWord < numWords; xWord++) {
            word = A->data[row*numWords + xWord];
            for(bit = 0; bit < 64 && word; bit++) {
                if(word & 1) {
                    xorMatrixRows(B, res, (xWord << 6) + bit, row);
                }
                word >>= 1;
            }
        }
    }
    return res;
}

// Xor the source row data onto the dest row data.
static void xorRowData(
    uint64 *sourceRow,
    uint64 *destRow)
{
    int i;

    for(i = 0; i < numWords; i++) {
        *destRow |= *sourceRow;
    }
}

// Multiply a vector on the left by a matrix on the right.
Bignum vectorMultiplyMatrix(Bignum v, Matrix A)
{
    Bignum res = createBignum(0, getBignumSize(v));
    int row;
    uint64 *resData = getBignumData(res);
    uint64 *AData = A->data;

    for(row = 0; row < N; row++) {
        if(getBignumBit(v, row)) {
            xorRowData(AData, resData);
        }
        AData += numWords;
    }
    return res;
}

// Multiply a matrix by a Bignum vector.  We assum it's vertical and on the right.
Bignum matrixMultiplyVector(Matrix A, Bignum n)
{
    Bignum res = createBignum(0, getBignumSize(n));
    int row;

    for(row = 0; row < N; row++) {
        setBignumBit(res, row, dotProdVect(A, n, row));
    }
    return res;
}

// Compute M^n.
Matrix matrixPow(
    Matrix M,
    Bignum n)
{
    Matrix res = identity();
    int size = getBignumSize(n);
    int i;

    for(i = 0; i < size; i++) {
        if(getBignumBit(n, i)) {
            res = matrixMultiply(res, M);
        }
        M = matrixMultiply(M, M);
    }
    return res;
}

static byte xorSum(uint64 n)
{
    byte value = 0;

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
    unsigned i;

    for(i = 0; i < (1 << 16); i++) {
        parityTable[i] = xorSum(i);
    }
}

// Starts looking at row = pos, col = pos, and searches down.
static int findNonZeroRow(Matrix M, int pos)
{
    int row;

    for(row = pos; row < N; row++) {
        if(getBit(M, row, pos)) {
            return row;
        }
    }
    return -1;
}

// XOR the source row into the dest row.
static inline void xorRow(Matrix M, int source, int dest)
{
    uint64 *s = M->data + source*numWords;
    uint64 *d = M->data + dest*numWords;
    int i;

    for(i = 0; i < numWords; i++) {
        *d++ ^= *s++;
    }
}

static bool isSingular(Matrix M)
{
    Matrix A = copy(M);
    int pos, row;

    for(pos = 0; pos < N; pos++) {
        row = findNonZeroRow(A, pos); // Starts looking at row = pos, col = pos
        if(row == -1) {
            return true;
        }
        if(row > pos) {
            xorRow(A, row, pos);
        }
        for(row = pos + 1; row < N; row++) {
            if(getBit(A, row, pos)) {
                xorRow(A, pos, row);
            }
        }
    }
    return false;
}

// Matrix inverse with basic Gaussian elimination.
// Start with A and I, and do Gaussian elimination to convert A to I,
// while doing the same operations to the other matrix.
Matrix inverse(Matrix M)
{
    Matrix I = identity();
    Matrix A = copy(M);
    int row, lowerRow, upperRow;

    // Do the row additions to zero out lower left of A
    for(row = 0; row < N; row++) {
        if(!getBit(A, row, row)) {
            lowerRow = findNonZeroRow(A, row);
            if(lowerRow == -1) {
                printf("Matrix is singular\n");
                return NULL;
            }
            xorRow(A, lowerRow, row);
            xorRow(I, lowerRow, row);
        }
        for(lowerRow = row + 1; lowerRow < N; row++) {
            if(getBit(A, lowerRow, row)) {
                xorRow(A, row, lowerRow);
                xorRow(I, row, lowerRow);
            }
        }
    }
    // Do the same thing to zero the upper part
    for(row = N - 1; row >= 0; row--) {
        for(upperRow = 0; upperRow < row; upperRow++) {
            if(getBit(A, upperRow, row)) {
                xorRow(A, row, upperRow);
                xorRow(I, row, upperRow);
            }
        }
    }
    return I;
}
    
// Create a random Boolean matrix.
static Matrix randomMatrix() 
{
    Matrix M = zero();
    int row, col;

    for(row = 0; row < N; row++) {
        for(col = 0; col < N; col++) {
            setBit(M, row, col, randomBool());
        }
    }
    return M;
}

// Create a random non-singular Boolean matrix.
static Matrix randomNonSingularMatrix(void)
{
    Matrix M, A;
    int i = 0;

    while(true) {
        M = randomMatrix();
        i += 1;
        if(!isSingular(M)) {
            //printf("Generated non-signular matrix in %d tries\n", i);
            A = add(M, identity());
            if(!isSingular(A)) {
                //printf("Found non-singular matrix with no eigan vectors in %d tries\n", i);
                return M;
            }
        }
    }
}

// Find if sequence A, A^2, A^4, ... , A^(2^(N-1)) has unique elements, and that A^(2^N) == A.
static bool hasGoodPowerOrder(Matrix A)
{
    Matrix M;
    HashTable hashTable = createHashTable(N);
    uint64 i;
    bool passed;

    A = allocateMatrix(A);
    if(isSingular(A)) {
        printf("Bug in finding non-singular matrix\n");
    }
    M = A;
    for(i = 0; i < N; i++) {
        if(lookupInHashTable(hashTable, M) != NULL) {
            delHashTable(hashTable);
            deleteMatrix(A);
            return false;
        }
        addToHashTable(hashTable, M);
        M = matrixMultiply(M, M);
    }
    passed = equal(M, A);
    if(isSingular(A)) {
        printf("Bug in finding non-singular matrix at %lld!\n", i);
    }
    delHashTable(hashTable);
    deleteMatrix(A);
    return passed;
}

// By "good", I mean the exponents A, A^2, A^4, ... A^(2^(N-1)) are unique, and
// A^(2^N) == A.
Matrix randomGoodMatrix(void)
{
    Matrix A;

    while(true) {
        A = randomNonSingularMatrix();
        if(hasGoodPowerOrder(A)) {
            return A;
        }
    }
    return NULL; // Dummy return
}

// Find the cycle length p of the matrix such that A^p == A.  Only search up to maxCycle.
// Return -1 if none found.
static uint64 findCycleLength(Matrix A, uint64 maxCycle)
{
    uint64 stepSize = (uint64)(sqrt((double)maxCycle) + 0.5);
    uint64 numSteps = (uint64)(maxCycle/stepSize);
    Matrix K = allocateMatrix(matrixPow(A, createBignum(stepSize, 64)));
    Matrix M = K;
    Matrix otherM, Atran;
    HashTable hashTable = createHashTable(numSteps);
    uint64 i, power, lowestPower;
    bool foundCollision = false;
    bool foundHit = false;

    A = allocateMatrix(A);
    Atran = allocateMatrix(transpose(A));
    //printf("populating hash table\n");
    for(i = 0; i < numSteps && !foundCollision; i++) {
        M->power = (i+1)*stepSize;
        otherM = lookupInHashTable(hashTable, M);
        if(otherM != NULL) {
            if(!foundCollision) {
                //printf("Found collision while building table: A^%d == A^%d\n",
                    //otherM->power, M->power);
                foundCollision = true;
            }
        } else {
            addToHashTable(hashTable, M);
        }
        M = matrixMultiply(M, K);
    }
    M = identity();
    //printf("Looking for hit.\n");
    lowestPower = -1;
    for(i = 0; i < stepSize; i++) {
        otherM = lookupInHashTable(hashTable, M);
        if(otherM != NULL) {
            power = otherM->power - i;
            foundHit = true;
            if(lowestPower == -1 || power < lowestPower) {
                lowestPower = power;
                //printf("Found cycle length of %d\n", power);
                //printf("M\n");
                //show(M);
                //printf("A^%d\n", otherM->power);
                //show(matrixPow(A, otherM->power));
                //printf("A^%d\n", power);
                //show(matrixPow(A, power));
            }
        }
        //printf("i is %d\n", i);
        //show(M);
        M = multiplyTransposed(M, Atran);
    }
    if(!foundHit) {
        printf("Looks like no loops below %lld\n", maxCycle);
    //} else {
        //printf("Original A\n");
        //show(A);
        //printf("A^%d\n", lowestPower);
        //show(matrixPow(A, lowestPower));
    }
    deleteMatrix(A);
    deleteMatrix(Atran);
    deleteMatrix(K);
    delHashTable(hashTable);
    return lowestPower;
}

void powTest(void)
{
    Matrix A, Am, An, key1M, key2M;
    Bignum m, n, key1V, key2V;

    initRandomModule();
    A = allocateMatrix(randomGoodMatrix());
    m = createBignum(randomUint64(), N);
    n = createBignum(randomUint64(), N);

    Am = allocateMatrix(matrixPow(A, m));
    An = allocateMatrix(matrixPow(A, n));
    key1M = allocateMatrix(matrixPow(Am, n));
    key2M = allocateMatrix(matrixPow(An, m));
    if(!equal(key1M, key2M)) {
        printf("Failed A^(m*n) test.\n");
    }
    key1V = matrixMultiplyVector(Am, n);
    key2V = matrixMultiplyVector(Am, n);
    if(!bignumsEqual(key1V, key2V)) {
        printf("Failed A^(m+n) test.\n");
    }
    deleteMatrix(key1M);
    deleteMatrix(key2M);
    deleteMatrix(A);
    deleteMatrix(Am);
    deleteMatrix(Am);
    deleteBignum(key1V);
    deleteBignum(key2V);
    printf("Passed pow test.\n");
}

static uint64 simpleFindCycleLength(Matrix A, long long maxCycle)
{
    Matrix origA = allocateMatrix(A);
    uint64 i;

    for(i = 1; i < maxCycle; i++) {
        A = matrixMultiply(A, origA);
        if(isSingular(A)) {
            printf("Singular matrix found!\n");
        }
        if(equal(A, origA)) {
            //printf("Cycle length is %d\n", i);
            deleteMatrix(origA);
            return i;
        }
    }
    deleteMatrix(origA);
    printf("Cycle length is > %lld\n", maxCycle);
    return -1;
}

// max order matrix, then test passes for N.
static bool checkPrimeOrderTheory(void)
{
    Matrix A;
    uint64 i, order;
    uint64 passes = 0;

    for(i = 0; i < 1000; i++) {
        A = randomNonSingularMatrix();
        A = allocateMatrix(A);
        if(hasGoodPowerOrder(A)) {
            order = findCycleLength(A, 1LL << (N + 1));
            if(order != (1 << N) - 1) {
                deleteMatrix(A);
                return false;
            }
            passes++;
            printf("Passed %lld times.\n", passes);
        }
        deleteMatrix(A);
    }
    return true;
}

// Initialize the matricies in the temporary queue.
static void initQueue(void)
{
    int i;

    for(i = 0; i < QUEUE_LEN; i++) {
        matrixQueue[i] = allocateMatrix(NULL);
    }
}

void initMatrixModule(int width)
{
    setWidth(width);
    initParityTable();
    initQueue();
}

// Reconstruct the user's matrix from his published first row.  We use the
// fact that the user's matrix H is communitive with G, HG = GH.  We know G, and
// the first row of H, called h.  Let's call the first row of G g.  hG == gH.
// We can compute hG, and restrict values of unknowns in H with these N linear
// equations.  Repeat this with G^2, G^3, ... until we have N - 1 linearly
// independent equations.  Then, solve for H.
Matrix reconstructMatrix(Matrix G, Bignum h)
{
    Matrix H = zero();
    Matrix R = zero(); // We will store various values of g here
    Matrix L = zero(); // We will store various values of gH (computed as hG) here
    Matrix M = copy(G);
    Bignum g, v;
    int i = 1;

    setRow(H, 0, h);
    setRow(R, 0, h);
    setRow(L, 0, createBignum(1, N));
    while(i < N) {
        g = getRow(M, 0);
        v = vectorMultiplyMatrix(g, M);
        //if(linearlyIndependent(H, i, v)) {
            setRow(L, i, h);
            setRow(R, i, v);
            i++;
            M = matrixMultiply(M, G);
        //}
    }
    return matrixMultiply(L, inverse(R));
}
