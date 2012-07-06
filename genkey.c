// Get random data from user as he types on keyboard randomly.  Once we have 127
// bits worth, save this as his private key k, and compute his public key as
// A^k*O.

#include <stdio.h>
#include "bmat.h"
#include "A127.h"

// Write the key in binary to the file.
static bool writeKey(char *fileName, uint64 *key)
{
    int i, numBytes;
    int requiredBytes = (N + 7)/8;
    int givenBytes = 8*numWords;
    int lastWordNumBytes = 8 - (givenBytes - requiredBytes);
    FILE *file = fopen("id_bmat", "wb");

    if(file == NULL) {
        printf("Unable t write to file id_bmat.\n");
        return 1;
    }
    for(i = 0; i < numWords; i++) {
        if(i + 1 == numWords) {
            // May not need to write the whole thing.
            numBytes = 
        }
        if(fwrite(key, , sizeof(uint64), FILE *FP);
}

int main(int argv, char **argv)
{
    Matrix A127;
    byte *privateKey, *publicKey;

    initMatrixModule(127);
    A127 = createMatrix(A127_data);
    privateKey = createPrivateKey();
    publicKey = getMatrixColumn(matrixPow(A127, privateKey), 0);
    if(!writeKey("id_bmat", privateKey)) {
        return 1;
    }
    if(!writeKey("id_bmat.pub", publicKey)) {
        return 1;
    }
    return 0;
}
