#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "bmat.h" // We use ARC4 for a random number generator

static int N; // Width of matrices.
static int numWords; // How many uint64 words are in each row.

// This is one megabyte of random data from 2012-07-06 at random.org
static byte *randomValues;
static uint64 numRandomValues;
static uint64 nextRandomValue = 0;
static int nextRandomBit = 0;

static Matrix firstFreeMatrix = NULL; // I'll maintain a free list of matricies.

// This table is for computing the parity of bits of 16-bit ints.
static byte parityTable[1 << 16];

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

// Hopefully random enough function, using ARC4 XORed on top of some actual
// random data, but the random data gets reused if there's not enough of it.
static inline byte randomByte(void)
{
    if(nextRandomValue == numRandomValues) {
        nextRandomValue = 0;
    }
    nextRandomBit = 0;
    return hashChar(randomValues[nextRandomValue++]);
}

static inline bool randomBool(void)
{
    if(nextRandomBit == 8) {
        nextRandomBit = 0;
        nextRandomValue++;
        if(nextRandomValue == numRandomValues) {
            nextRandomValue = 0;
        }
    }
    return parityTable[hashChar(0)] ^
        ((randomValues[nextRandomValue] >> nextRandomBit++) & 1);
}

// Return 8 random bytes in a uint64.
static inline uint64 randomUint64(void)
{
    uint64 value = 0LL;
    int i;

    for(i = 0; i < 8; i++) {
        value |= (uint64)(randomByte()) << 8*i;
    }
    return value;
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

static inline Matrix allocate(Matrix oldM)
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

    M = allocate(M);
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
    return allocate(M);
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

    printf("uint64 A%d_data = {\n", N);
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
static Matrix multiply(Matrix A, Matrix B)
{
    return multiplyTransposed(A, transpose(B));
}

// Compute M^n.
Matrix matrixPow(
    Matrix M,
    Bignum n)
{
    Matrix res = identity();
    int i;

    for(i = 0; i < n->bits; i++) {
        if(getBignumBit(n, i)) {
            res = multiply(res, M);
        }
        M = multiply(M, M);
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

    A = allocate(A);
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
        M = multiply(M, M);
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
static Matrix randomGoodMatrix(void)
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
    Matrix K = allocate(matrixPow(A, makeBignum(stepSize, 64)));
    Matrix M = K;
    Matrix otherM, Atran;
    HashTable hashTable = createHashTable(numSteps);
    uint64 i, power, lowestPower;
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
    deleteMatrix(A);
    deleteMatrix(Atran);
    deleteMatrix(K);
    delHashTable(hashTable);
    return lowestPower;
}

static void powTest(void)
{
    Matrix A = allocate(randomNonSingularMatrix());
    Matrix key1, key2;
    Bignum n = makeBignum(randomUint64(), 64);
    Bignum m = makeBignum(randomUint64(), 64);

    key1 = allocate(matrixPow(matrixPow(A, m), n));
    key2 = allocate(matrixPow(matrixPow(A, n), m));
    if(equal(key1, key2)) {
        printf("Passed pow test.\n");
    } else {
        printf("Failed pow test.\n");
    }
    deleteMatrix(key1);
    deleteMatrix(key2);
    deleteMatrix(A);
}

static uint64 simpleFindCycleLength(Matrix A, long long maxCycle)
{
    Matrix origA = allocate(A);
    uint64 i;

    for(i = 1; i < maxCycle; i++) {
        A = multiply(A, origA);
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
        A = allocate(A);
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

static void readRandomData(
    char *fileName)
{
    FILE *file = fopen(fileName, "r");
    int c, i = 0, j;
    byte b;

    numRandomValues = 1 << 20; // Lucky guess!
    randomValues = (byte *)calloc(numRandomValues, sizeof(byte));
    if(file == NULL) {
        printf("Unable to rad random.txt\n");
        return;
    }
    c = getc(file);
    while(c != EOF) {
        if(i == numRandomValues) {
            numRandomValues <<= 1;
            randomValues = (byte *)realloc(randomValues, numRandomValues*sizeof(byte));
        }
        b = 0;
        for(j = 0; c != EOF && j < 8; j++) {
            c = getc(file);
            if(c == '1') {
                b |= 1 << j;
            }
        }
        if(c != EOF) {
            randomValues[i++] = b;
        }
    }
    fclose(file);
}

// Initialize the matricies in the temporary queue.
static void initQueue(void)
{
    int i;

    for(i = 0; i < QUEUE_LEN; i++) {
        matrixQueue[i] = allocate(NULL);
    }
}

void initMatrixModule(int width)
{
    setWidth(width);
    initParityTable();
    readRandomData("random.txt");
    for(i = 0; i < sizeof(password); i++) {
        do {
            c = randomByte();
        } while(c == '\0');
        password[i] = c;
    }
    password[sizeof(password) - 1] = '\0';
    initKey(password, NULL, 0);
    throwAwaySomeBytes(DISCARD_BYTES);
    powTest();
}

#if 0
int main()
{
    Matrix A;
    uint64 i;
    uint64 length, length2, maxLength = 0;
    uint64 equalMax = 0;
    uint64 total = 1000;
    uint64 maxCycle;
    char password[1024];
    byte c;

    setWidth(127);
    initParityTable();
    readRandomData("random.txt");
    for(i = 0; i < sizeof(password); i++) {
        do {
            c = randomByte();
        } while(c == '\0');
        password[i] = c;
    }
    password[sizeof(password) - 1] = '\0';
    initKey(password, NULL, 0);
    throwAwaySomeBytes(DISCARD_BYTES);

    initQueue();
    powTest();
    A = randomGoodMatrix();
    showMatrixInHex(A);
    return 0;

    for(N = 31; N <= 31; N++) {
        if(checkPrimeOrderTheory()) {
            printf("N %d passed\n", N);
        } else {
            printf("N %d failed\n", N);
        }
    }

    /*
    for(N = 40; N <= 40; N++) {
        maxLength = 0;
        maxCycle = 1LL << (N+1);
        for(i = 0; i < total && maxLength != (1LL << N) - 1LL; i++) {
            printf("Try %lld\n", i);
            A = allocate(randomNonSingularMatrix());
            //length = simpleFindCycleLength(A, maxCycle);
            length = findCycleLength(A, maxCycle);
            //length = simpleFindCycleLength(A, maxCycle);
            if(length > maxLength) {
                maxLength = length;
            }
            deleteMatrix(A);
        }
        printf("For N=%lld, length=%lld, found in %lld tries\n", N, maxLength, i);
        showMatrix(A);
        //if(length != length2) {
            //length2 = findCycleLength(A, maxCycle);
            //printf("Errors in findCycleLength!\n");
        //}
        //if(maxLength < length) {
            //maxLength = length;
            //equalMax = 0;
        //}
        //if(length == maxLength) {
            //equalMax++;
        //}
        //if((i % (total/100)) == 0) {
            //printf("%lld: maxLength=%lld, length = %lld\n", i, maxLength, length);
        //}
        //deleteMatrix(A);
    }
    printf("Max: %lld, percent equal to max: %f\n", maxLength, (100.0*equalMax)/total);
    */
    return 0;
}
#endif
