#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmat.h"

#define MAX_KEY_LENGTH (1 << 14)

struct BignumStruct {
    uint64 *data;
    int bits;
    bool isPrivate;
};

bool getBignumBit(Bignum n, int bit)
{
    return (n->data[bit >> 6] >> (bit & 0x3f)) & 1;
}

void setBignumBit(Bignum n, int bit, bool value)
{
    if(value) {
        n->data[bit >> 6] |= 1LL << (bit & 0x3f);
    } else {
        n->data[bit >> 6] &= ~(1LL << (bit & 0x3f));
    }
}

byte getBignumByte(Bignum n, int i)
{
    int wordPos = i >> 3;
    int bytePos = i & 7;

    return (byte)(n->data[wordPos] >> bytePos*8);
}

void setBignumByte(Bignum n, int i, byte value)
{
    int wordPos = i >> 3;
    int bytePos = i & 7;

    n->data[wordPos] &=  ~((uint64)0xff << bytePos*8);
    n->data[wordPos] |= ((uint64)value) << bytePos*8;
}

uint64 getBignumWord(Bignum n, int i)
{
    return n->data[i];
}

Bignum createBignum(uint64 value, int bits)
{
    Bignum n = (Bignum)calloc(1, sizeof(struct BignumStruct));

    n->bits = bits;
    n->data = (uint64 *)calloc((bits + 63) >> 6, sizeof(uint64));
    n->data[0] = value;
    return n;
}

Bignum createBignumFromBytes(byte *bytes, int bits)
{
    Bignum n = createBignum(0, bits);
    int numWords = (bits + 63) >> 6;
    int numBytes = (bits + 7) >> 3;
    uint64 word;
    int i, j;

    n->bits = bits;
    for(i = 0; i < numWords; i++) {
        word = 0;
        for(j = 0; j < 8 && i*8 + j < numBytes; j++) {
            word |= (uint64)(*bytes++) << j*8;
        }
        n->data[i] = word;
    }
    return n;
}

void deleteBignum(Bignum n)
{
    free(n->data);
    free(n);
}

int getBignumSize(Bignum n)
{
    return n->bits;
}

void showBignum(Bignum n)
{
    int numBytes = (n->bits + 7)/8;
    uint64 word;
    int i;

    for(i = 0; i < numBytes; i++) {
        word = n->data[i >> 3];
        printf("%02x", (byte)(word >> 8*(i & 7)));
    }
    printf("\n");
}

// Write the key in binary to the file.  The first 15 bits encode key length,
// the 16th bit is the private key flag, and the rest is the key.
bool writeKey(char *fileName, Bignum key, bool isPrivate)
{
    FILE *file = fopen(fileName, "wb");
    int bits = getBignumSize(key);
    int numBytes = (bits + 7)/8 + 2; // First two bytes encode length in bits.
    byte buffer[numBytes];
    int i;

    if(file == NULL) {
        printf("Unable to write to file %s.\n", fileName);
        return false;
    }
    buffer[0] = (byte)key->bits;
    buffer[1] = (byte)(key->bits >> 8);
    if(isPrivate) {
        buffer[1] |= 0x80;
    }
    for(i = 0; i < numBytes - 2; i++) {
        buffer[i + 2] = getBignumByte(key, i);
    }
    if(fwrite(buffer, sizeof(byte), numBytes, file) != numBytes) {
        printf("Unable to write key to %s\n", fileName);
        return false;
    }
    fclose(file);
    return true;
}

// Read the key in binary to the file.
Bignum readKey(char *fileName, bool isPrivateKey)
{
    FILE *file = fopen(fileName, "rb");
    byte buffer[MAX_KEY_LENGTH];
    bool fileIsPrivateKey;
    unsigned numRead, pos = 0, bits;

    if(file == NULL) {
        printf("Unable to read from file %s.\n", fileName);
        return NULL;
    }
    do {
        numRead = fread(buffer + pos, sizeof(byte), MAX_KEY_LENGTH, file);
        pos += numRead;
    } while(numRead > 0);
    fclose(file);
    bits = ((unsigned)buffer[0]) | (((unsigned)buffer[1] & 0x7) << 8);
    if(pos != (bits + 7)/8 + 2) {
        printf("Key file %s has incorrect size.\n", fileName);
        return NULL;
    }
    fileIsPrivateKey = (buffer[1] >> 7) != 0;
    if(fileIsPrivateKey && !isPrivateKey) {
        printf("Trying to read public key from private key file %s\n", fileName);
        return NULL;
    }
    if(!fileIsPrivateKey && isPrivateKey) {
        printf("Trying to read private key from public key file %s\n", fileName);
        return NULL;
    }
    return createBignumFromBytes(buffer + 2, bits);
}

// Check if to bignums are equal.
bool bignumsEqual(Bignum n, Bignum m)
{
    int numWords = (n->bits + 63) >> 6;
    int i;

    if(n->bits != m->bits) {
        return false;
    }
    for(i = 0; i < numWords; i++) {
        if(n->data[i] != m->data[i]) {
            return false;
        }
    }
    return true;
}

// Set the isPrivate flag on the bignum.
void bignumSetIsPrivateKey(Bignum n)
{
    n->isPrivate = true;
}

uint64 *getBignumData(Bignum n)
{
    return n->data;
}
