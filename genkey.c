// Get random data from user as he types on keyboard randomly.  Once we have 127
// bits worth, save this as his private key k, and compute his public key as
// O*G^k.

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

// Generate a private key from random data in /dev/random.
static Bignum createPrivateKeyFromDevRandom(
    int bits)
{
    Bignum n;
    FILE *randFile = fopen("/dev/urandom", "r");
    int i, remaining;

    if(randFile == NULL) {
        printf("Unable to open random number source.\n");
        return NULL;
    }
    n = createBignum(0, bits);
    for(i = 0; i < bits/8; i++) {
        setBignumByte(n, i, getc(randFile));
    }
    remaining = bits - (bits/8)*8;
    if(remaining > 0) {
        setBignumByte(n, i, getc(randFile) >> (8 - remaining));
    }
    return n;
}

// Ask the user to type for a while to generate a random private key.
static Bignum createPrivateKeyFromKeyboard(
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
    Matrix G, H;
    Bignum privateKey, publicKey;
    Bignum readPrivateKey, readPublicKey;
    int N = 127;
    int xArg = 1;
    char fileName[123];
    bool useDevRandom = false;

    
    while(xArg < argc && argv[xArg][0] == '-') {
        if(!strcmp(argv[xArg], "-r")) {
            useDevRandom = true;
        }
        xArg++;
    }
    if(xArg + 1 == argc) {
        N = atoi(argv[xArg]);
        if(N == 0) {
            printf("Usage: genkey [-r] <length>.\n"
               "    -r : Use /dev/random rather than keyboard input for random data\n");
        }
    }
    G = getGenerator(N);
    N = getMatrixSize();
    printf("Using %d bit key length.\n", N);
    if(useDevRandom) {
        privateKey = createPrivateKeyFromDevRandom(N);
    } else {
        privateKey = createPrivateKeyFromKeyboard(N);
    }
    H = matrixPow(G, privateKey);
    showMatrixInHex(H);
    publicKey = getMatrixRow(H, 0);
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
