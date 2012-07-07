#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmat.h"

struct BignumStruct {
    uint64 *data;
    int bits;
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

// Write the key in binary to the file.
bool writeKey(char *fileName, Bignum key)
{
    FILE *file = fopen(fileName, "wb");
    int bits = getBignumSize(key);
    int numBytes = (bits + 7)/8;
    byte buffer[numBytes];
    int i;

    if(file == NULL) {
        printf("Unable to write to file %s.\n", fileName);
        return false;
    }
    for(i = 0; i < numBytes; i++) {
        buffer[i] = getBignumByte(key, i);
    }
    if(fwrite(buffer, sizeof(byte), numBytes, file) != numBytes) {
        printf("Unable to write key to %s\n", fileName);
        return false;
    }
    fclose(file);
    return true;
}

// Read the key in binary to the file.
Bignum readKey(char *fileName, int bits)
{
    FILE *file = fopen(fileName, "rb");
    int numBytes = (bits + 7) >> 3;
    byte buffer[numBytes];
    int numRead;

    if(file == NULL) {
        printf("Unable to write to file %s.\n", fileName);
        return NULL;
    }
    numRead = fread(buffer, sizeof(byte), numBytes, file);
    if(numRead != numBytes) {
        printf("Unable to read all the bytes required from %s", fileName);
        fclose(file);
        return NULL;
    }
    fclose(file);
    return createBignumFromBytes(buffer, bits);
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
