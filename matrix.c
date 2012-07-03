#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

// If setting WIDTH to 64, also change the dotProd function.
#define WIDTH 32
static unsigned N = WIDTH;

typedef unsigned long Row;

typedef struct MatrixStruct *Matrix;
typedef struct HashTableStruct *HashTable;

struct MatrixStruct {
    Row rowVal[WIDTH];
    Matrix nextMatrix;
    unsigned power; // Written to matrices in hash table.
};

static Matrix firstFreeMatrix = NULL; // I'll maintain a free list of matricies.

struct HashTableStruct {
    unsigned size;
    Matrix *matrices;
};

// This table is for computing the parity of bits of 16-bit ints.
static unsigned char parityTable[1 << 16];

// This is a queue of temporary matrices, which get reused after a while.  To
// allocate a matrix permanently, call copy.
#define QUEUE_LEN 256
static struct MatrixStruct matrixQueue[QUEUE_LEN];
static int queuePos = 0;

// Hash table functions.

static inline unsigned hashValues(unsigned hash1, unsigned hash2)
{
    return hash1 ^ (hash2*1103515245 + 12345);
}

static unsigned hashMatrix(Matrix M)
{
    int i;
    unsigned int hash = 0;

    for(i = 0; i < N; i++) {
        hash = hashValues(hash, M->rowVal[i]);
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

    for(i = 0; i < N; i++) {
        if(A->rowVal[i] != B->rowVal[i]) {
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

static inline Matrix allocate(Matrix oldM)
{
    Matrix M = firstFreeMatrix;
    static int totalAllocated = 0;

    if(M != NULL) {
        firstFreeMatrix = M->nextMatrix;
        M->nextMatrix = NULL;
    } else {
        M = (Matrix)malloc(sizeof(struct MatrixStruct));
        totalAllocated++;
        if((totalAllocated % 1000) == 0) {
            printf("Allocated %d matrices\n", totalAllocated);
        }
    }
    memcpy((void *)M, (void *)oldM, sizeof(struct MatrixStruct));
    M->nextMatrix = NULL;
    return M;
}

static void addToHashTable(HashTable hashTable, Matrix M)
{
    unsigned index = hashMatrix(M) % hashTable->size;
    Matrix otherM = hashTable->matrices[index];

    M = allocate(M);
    M->nextMatrix = otherM;
    hashTable->matrices[index] = M;
}

static inline void delete(Matrix M)
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
            delete(M);
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
    return matrixQueue + queuePos;
}

static inline Matrix copy(Matrix oldM)
{
    Matrix M = newMatrix();

    memcpy((void *)M, (void *)oldM, sizeof(struct MatrixStruct));
    M->nextMatrix = NULL;
    return M;
}

static inline void setBit(Matrix M, int row, int col, int value)
{
    if(value) {
        M->rowVal[row] |= 1 << col;
    } else {
        M->rowVal[row] &= ~(1 << col);
    }
}

static inline int getBit(Matrix M, int row, int col)
{
    return (M->rowVal[row] >> col) & 1;
}

static Matrix zero(void)
{
    Matrix M = newMatrix();

    memset((void *)M, 0, sizeof(struct MatrixStruct));
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

static void show(Matrix M)
{
    int row, col;

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
static int dotProd(Matrix A, Matrix B, int row, int col)
{
    Row vect = A->rowVal[row] & B->rowVal[col];

    // Note: this must be changed if working with 64-bit!
    return parityTable[vect >> 16] ^ parityTable[vect & 0xffff];
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

// This is much slower, since it has to transpose N first.
static Matrix multiply(Matrix A, Matrix B)
{
    return multiplyTransposed(A, transpose(B));
}

// Compute M^n.
static Matrix matrixPow(
    Matrix M,
    unsigned n)
{
    Matrix res = identity();
    Matrix powers[WIDTH];
    int bit = 1;

    powers[0] = M;
    while(bit < 32 && (1 << bit) <= n) {
        powers[bit] = multiply(powers[bit-1], powers[bit-1]);
        bit += 1;
    }
    bit = 0;
    while(n != 0) {
        if(n & 1) {
            res = multiply(res, powers[bit]);
        }
        bit += 1;
        n >>= 1;
    }
    return res;
}

static unsigned char xorSum(unsigned n)
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
    M->rowVal[dest] ^= M->rowVal[source];
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
    
// Create a random Boolean matrix.
static Matrix randomMatrix() 
{
    Matrix M = zero();
    int row, col;

    for(row = 0; row < N; row++) {
        for(col = 0; col < N; col++) {
            setBit(M, row, col, rand() & 0x1);
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
            if(isSingular(A)) {
                //printf("Found non-singular matrix with no eigan vectors in %d tries\n", i);
                return M;
            }
        }
    }
}

// Find the cycle length p of the matrix such that A^p == A.  Only search up to maxCycle.
// Return -1 if none found.
static unsigned findCycleLength(Matrix A, long long maxCycle)
{
    unsigned stepSize = (unsigned)(sqrt((double)maxCycle) + 0.5);
    unsigned numSteps = (unsigned)maxCycle/stepSize;
    Matrix K = allocate(matrixPow(A, stepSize));
    Matrix M = K;
    Matrix otherM, Atran;
    HashTable hashTable = createHashTable(numSteps);
    unsigned i, power, lowestPower;
    bool foundCollision = false;
    bool foundHit = false;

    A = allocate(A);
    Atran = allocate(transpose(A));
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
        M = multiply(M, K);
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
    delete(A);
    delete(Atran);
    delete(K);
    delHashTable(hashTable);
    return lowestPower;
}

static void powTest(void)
{
    Matrix A = randomNonSingularMatrix();
    Matrix key1, key2;
    unsigned n = rand();
    unsigned m = rand();

    key1 = matrixPow(matrixPow(A, m), n);
    key2 = matrixPow(matrixPow(A, n), m);
    if(equal(key1, key2)) {
        printf("Passed pow test.\n");
    } else {
        printf("Failed pow test.\n");
    }
}

static unsigned simpleFindCycleLength(Matrix A, long long maxCycle)
{
    Matrix origA = allocate(A);
    unsigned i;

    for(i = 1; i < maxCycle; i++) {
        A = multiply(A, origA);
        if(isSingular(A)) {
            printf("Singular matrix found!\n");
        }
        if(equal(A, origA)) {
            //printf("Cycle length is %d\n", i);
            delete(origA);
            return i;
        }
    }
    delete(origA);
    printf("Cycle length is > %lld\n", maxCycle);
    return -1;
}

int main()
{
    Matrix A;
    unsigned i;
    unsigned length, length2, maxLength = 0;
    unsigned equalMax = 0;
    unsigned total = 100;
    unsigned maxCycle = 2200000000LL;

    initParityTable();
    N = 32;
    powTest();

    for(i = 1; i < total; i++) {
        A = allocate(randomNonSingularMatrix());
        //printf("Original matrix\n");
        //show(A);
        //length = simpleFindCycleLength(A, maxCycle);
        //printf("Cycle length is %d\n", length);
        //show(matrixPow(A, length));
        length = findCycleLength(A, maxCycle);
        //if(length != length2) {
            //length2 = findCycleLength(A, maxCycle);
            //printf("Errors in findCycleLength!\n");
        //}
        if(maxLength < length) {
            maxLength = length;
            equalMax = 0;
        }
        if(length == maxLength) {
            equalMax++;
        }
        if((i % (total/100)) == 0) {
            printf("%d: maxLength=%d, length = %d\n", i, maxLength, length);
        }
        delete(A);
    }
    printf("Max: %d, percent equal to max: %f\n", maxLength, (100.0*equalMax)/total);
    return 0;
}
