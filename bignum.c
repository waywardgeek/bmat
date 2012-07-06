#include "bmat.h"

struct BignumStruct {
    byte *data;
    int bits;
};

bool getBignumBit(Bignum n, int bit)
{
    return n->data[bit >> 3] >> (bit & 0x7) & 1;
}

void setBignumBit(Bignum n, int bit, bool value)
{
    if(value) {
        n->data[bit >> 3] |= 1 << (bit & 7);
    } else {
        n->data[bit >> 3] &= ~(1 << (bit & 7));
    }
}

Bignum makeBignum(uint64 value, int bits)
{
    Bignum n = (Bignum)calloc(1, sizeof(struct BignumStruct));
    int i;

    n->bits = bits;
    n->data = (byte *)calloc((bits + 7) >> 3, sizeof(byte));
    for(i = 0; i < bits; i++) {
        setBignumBit(n, i, (value >> i) & 1);
    }
    return n;
}

static void deleteBignum(Bignum n)
{
    free(n->data);
    free(n);
}

