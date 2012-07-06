/* Based on ARC4-DROP(1024).  See http://en.wikipedia.org/wiki/RC4. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "boolenc.h"

static byte temp;
#define swap(a, b) (temp = (a), (a) = (b), (b) = temp)

static byte S[KEY_LENGTH]; /* Note: KEY_LENGTH MUST be 256! */
static byte i, j;
static int xChar;

/* Hash the character using the current key position, and modify the key. */
byte hashChar(byte c) {
    i++;
    j += S[i];
    swap(S[i], S[j]);
    return c ^ S[(byte)(S[i] + S[j])];
}

/* Just set the key to the identity permutation. */
void clearKey(void) {
    for(xChar = 0; xChar < KEY_LENGTH; xChar++) {
        S[xChar] = xChar;
    }
}

/* Throw away the first DISCARD_BYTES bytes, since they correlate to the key. */
void throwAwaySomeBytes(int numBytes) {
    for(xChar = 0; xChar < numBytes; xChar++) {
        hashChar('\0');
    }
}

/* Initialize the key from the password, as done in ARC4. */
void initKey(char *password, byte *nonce, int nonceLength) {
    char *p = password;

    clearKey();
    j = 0;
    for(xChar = 0; xChar < KEY_LENGTH; xChar++) {
        if(*p == '\0') {
            p = password;
        }
        j += S[xChar] + *p++;
        swap(S[xChar], S[j]);
    }
    for(xChar = 0; xChar < nonceLength; xChar++) {
        j += S[i] + *nonce++;
        swap(S[i], S[j]);
        i++;
    }
    i = 0;
    j = 0;
}
