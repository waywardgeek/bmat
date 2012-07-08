// Just test the random number generator a bit.
#include <stdio.h>
#include <stdlib.h>
#include "bmat.h"

// This is one megabyte of random data from 2012-07-06 at random.org
static byte *randomValues;
static uint64 numRandomValues;
static uint64 nextRandomValue = 0;
static int nextRandomBit = 0;
static bool initialized = false;

// Hopefully random enough function, using ARC4 XORed on top of some actual
// random data, but the random data gets reused if there's not enough of it.
byte randomByte(void)
{
    if(nextRandomValue == numRandomValues) {
        nextRandomValue = 0;
    }
    nextRandomBit = 0;
    return hashChar(randomValues[nextRandomValue++]);
}

bool randomBool(void)
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
uint64 randomUint64(void)
{
    uint64 value = 0LL;
    int i;

    for(i = 0; i < 8; i++) {
        value |= (uint64)(randomByte()) << 8*i;
    }
    return value;
}

static void readASCIIRandomData(
    char *fileName)
{
    FILE *file = fopen(fileName, "r");
    int c, i = 0, j;
    byte b;

    numRandomValues = 1 << 20; // Lucky guess!
    randomValues = (byte *)calloc(numRandomValues, sizeof(byte));
    if(file == NULL) {
        printf("Unable to read %s\n", fileName);
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

// Read random data from a binary file.
static void readRandomData(
    char *fileName)
{
    FILE *file = fopen(fileName, "rb");
    unsigned numRead, totalRead = 0;

    numRandomValues = 1 << 20; // Lucky guess!
    randomValues = (byte *)calloc(numRandomValues, sizeof(byte));
    if(file == NULL) {
        printf("Unable to read %s\n", fileName);
        return;
    }
    do {
        numRead = fread(randomValues + totalRead, sizeof(byte),
            numRandomValues - totalRead, file);
        totalRead += numRead;
    } while(numRead > 0);
    fclose(file);
}

// Save the random data in binary.
static void writeRandomData(char *fileName)
{
    FILE *file = fopen(fileName, "wb");

    if(file == NULL) {
        printf("Unable to write to file %s\n", fileName);
        return;
    }
    if(fwrite(randomValues, sizeof(byte), numRandomValues, file) != numRandomValues) {
        printf("Unable to write %s\n", fileName);
        fclose(file);
        return;
    }
    fclose(file);
}

void initRandomModule(void)
{
    char password[1024];
    byte c;
    int i;

    if(initialized) {
        return;  // Already initilaized;
    }
    readRandomData("random.bin");
    for(i = 0; i < sizeof(password); i++) {
        do {
            c = randomByte();
        } while(c == '\0');
        password[i] = c;
    }
    password[sizeof(password) - 1] = '\0';
    initKey(password, NULL, 0);
    throwAwaySomeBytes(DISCARD_BYTES);
    initialized = true;
}
