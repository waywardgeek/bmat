// Get random data from user as he types on keyboard randomly.  Once we have 127
// bits worth, save this as his private key k, and compute his public key as
// G^k*O.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "bmat.h"
#include "generators.h"

static byte counter;
static volatile bool quit = false;

static void *runCounter(void *ptr)
{
    while(!quit) {
        counter++;
    }
    return NULL;
}

// Ask the user to type for a while to generate a random private key.
static Bignum createPrivateKey(
    int bits)
{
    Bignum key = createBignum(0, bits);
    int i = 0, j, c;
    byte count;
    pthread_t thread1;

    pthread_create(&thread1, NULL, runCounter, NULL);
    printf("Please hit the enter key repeatedly for while to generate random data.");
    while(i < bits) {
        do {
            c = getchar();
        } while(c != '\n');
        count = counter;
        for(j = 0; j < 8 && i < bits; j++) {
            setBignumBit(key, i++, (count >> j) & 1);
            c >>= 1;
        }
        printf("%d of %d completed", i, bits);
    }
    printf("\nDone generating random data.  Building public key...\n");
    quit = true;
    pthread_join(thread1, NULL);
    return key;
}

int main(int argc, char **argv)
{
    Matrix G;
    Bignum privateKey, publicKey;
    Bignum readPrivateKey, readPublicKey;
    int N = 127;
    char fileName[123];

    if(argc == 2) {
        N = atoi(argv[1]);
        if(N == 0) {
            printf("Usage: genkey <length>.  Length must be 127, 521, or 607.\n");
        }
    }
    printf("Using %d bit key length.\n", N);
    G = getGenerator(N);
    privateKey = createPrivateKey(N);
    publicKey = getMatrixColumn(matrixPow(G, privateKey), 0);
    sprintf(fileName, "id_%d.priv", N);
    if(!writeKey(fileName, privateKey, true)) {
        return 1;
    }
    readPrivateKey = readKey(fileName, true);
    if(readPrivateKey == NULL || !bignumsEqual(privateKey, readPrivateKey)) {
        printf("Unable to read back private key\n");
        return 1;
    }
    printf("Wrote private key %s\n", fileName);
    sprintf(fileName, "id_%d.pub", N);
    if(!writeKey(fileName, publicKey, false)) {
        return 1;
    }
    readPublicKey = readKey(fileName, false);
    if(readPublicKey == NULL || !bignumsEqual(publicKey, readPublicKey)) {
        printf("Unable to read back private key\n");
        return 1;
    }
    printf("Wrote public key %s\n", fileName);
    return 0;
}
