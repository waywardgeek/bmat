// Just test the random number generator a bit.
#include <stdio.h>
#include "bmat.h"

int main()
{
    int i, j;

    for(j = 0; j < 24; j++) {
        for(i = 0; i < 40; i++) {
            printf("%02x", hashChar(0));
        }
        printf("\n");
    }
    return 0;
}
