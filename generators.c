#include <stdio.h>
#include "bmat.h"
#include "generators.h"
#include "G2.h"
#include "G3.h"
#include "G5.h"
#include "G7.h"
#include "G13.h"
#include "G17.h"
#include "G19.h"
#include "G31.h"
#include "G61.h"
#include "G89.h"
#include "G107.h"
#include "G127.h"
#include "G521.h"
#include "G607.h"

#define NUM_GENERATORS 14
static int generators[NUM_GENERATORS] =
{2, 3, 5, 7, 13, 17, 19, 31, 61, 89, 107, 127, 521, 607};
static uint64 *(generatorData[NUM_GENERATORS]) =
{G2_data, G3_data, G5_data, G7_data, G13_data, G17_data, G19_data, G31_data,
G61_data, G89_data, G107_data, G127_data, G521_data, G607_data};

// Return the generator of size N or just larger.
Matrix getGenerator(int N)
{
    uint64 *data = NULL;
    int i;

    for(i = 0; i < NUM_GENERATORS && generators[i] < N; i++);
    N = generators[i];
    data = generatorData[i];
    initMatrixModule(N);
    return allocateMatrix(createMatrix(data));
}
