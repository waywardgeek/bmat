// Get random data from user as he types on keyboard randomly.  Once we have 127
// bits worth, save this as his private key k, and compute his public key as
// A^k*O.

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "bmat.h"
#include "A127.h"
#include "A607.h"

static byte counter;

static byte counter;
static bool quit = false;

static void *runCounter(void *ptr)
{
    while(!quit) {
        counter++;
    }
    return NULL;
}

// Taunt the poor user.
static void printRandomIncentive(int bitsCompleted, int bitsNeeded, byte count)
{
    switch(count & 7) {
    case 0: printf("Oh, yeah!  Do it to me, baby!"); break;
    case 1: printf("Keep it comming!"); break;
    case 2: printf("Don't let me down now..."); break;
    case 3: printf("So close, yet so far..."); break;
    case 4: printf("You have joined the Dark Side!  You will burn in Hell!"); break;
    case 5: printf("I just broke a nail!"); break;
    case 6: printf("Please... please... please keep doing that."); break;
    case 7: printf("Almost there!  Almost there!"); break;
    }
    printf(" (%d of %d completed)", bitsCompleted, bitsNeeded);
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
        printRandomIncentive(i, bits, count);
    }
    quit = true;
    pthread_join(thread1, NULL);
    return key;
}

int main(int argc, char **argv)
{
    Matrix A;
    Bignum privateKey, publicKey, key;
    int N = 127;

    if(argc == 2) {
        if(!strcmp(argv[1], "-607")) {
            N = 607;
        }
    }
    initMatrixModule(N);
    if(N == 127) {
        A = createMatrix(A127_data);
    } else if(N == 607) {
        A = createMatrix(A607_data);
    }
    privateKey = createPrivateKey(N);
    publicKey = getMatrixColumn(matrixPow(A, privateKey), 0);
    if(!writeKey("id_bmat", privateKey)) {
        return 1;
    }
    if(!writeKey("id_bmat.pub", publicKey)) {
        return 1;
    }
    key = readKey("id_bmat", N);
    if(bignumsEqual(privateKey, key)) {
        printf("Passed key write/read test\n");
    } else {
        printf("Failed key write/read test!\n");
    }
    return 0;
}
