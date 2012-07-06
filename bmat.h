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
Bignum getMatrixColumn(Matrix A);
Matrix matrixPow(Matrix A, Bignum n);
Matrix createMatrix(uint64 *data);
void deleteMatrix(Matrix M);

// Bignum interface
bool getBignumBit(Bignum n, int bit);
void setBignumBit(Bignum n, int bit, bool value);
Bignum makeBignum(uint64 value, int bits);
