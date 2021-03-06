#include <stdbool.h>

// Type definitions
typedef unsigned char byte;
typedef unsigned long long uint64;
typedef struct MatrixStruct *Matrix;
typedef struct HashTableStruct *HashTable;
typedef struct BignumStruct *Bignum;

// ARC4 interface
#define KEY_LENGTH 256
#define NONCE_LENGTH 20
#define DISCARD_BYTES 1024
#define DIFFICULTY 200000 /* We throw away DISCARD_BYTES*DIFFICULTY bytes */

void initKey(char *password, byte *nonce, int nonceLength);
void throwAwaySomeBytes(int numBytes);
void clearKey(void);
byte hashChar(byte c);

// Matrix interface
void initMatrixModule(int width);
Matrix randomGoodMatrix(void);
void showMatrixInHex(Matrix A);
void showMatrix(Matrix A);
Bignum getMatrixColumn(Matrix A, int column);
Bignum getMatrixRow(Matrix A, int row);
Matrix matrixMultiply(Matrix A, Matrix B);
Matrix matrixPow(Matrix A, Bignum n);
int getMatrixSize(void);
Bignum matrixMultiplyVector(Matrix A, Bignum n);
Matrix createMatrix(uint64 *data);
void deleteMatrix(Matrix M);
void powTest(void);
Matrix allocateMatrix(Matrix oldM);
Matrix reconstructMatrix(Matrix G, Bignum h);
bool checkPrimeOrderTheory(void);
extern byte parityTable[1 << 16];

// Bignum interface
int getBignumSize(Bignum n);
bool getBignumBit(Bignum n, int bit);
void setBignumBit(Bignum n, int bit, bool value);
void setBignumByte(Bignum n, int i, byte value);
byte getBignumByte(Bignum n, int i);
uint64 getBignumWord(Bignum n, int i);
Bignum createBignum(uint64 value, int bits);
bool writeKey(char *fileName, Bignum key, bool isPrivateKey);
Bignum readKey(char *fileName, bool isPrivateKey);
bool bignumsEqual(Bignum n, Bignum m);
void showBignum(Bignum n);
void bignumSetIsPrivateKey(Bignum n);
void deleteBignum(Bignum n);
uint64 *getBignumData(Bignum n);

// PRNG random number generaor
void initRandomModule(bool randomize);
byte randomByte(void);
bool randomBool(void);
uint64 randomUint64(void);
