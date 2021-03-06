#CFLAGS=-g -Wall -Wno-unused
CFLAGS=-std=c99 -O3 -Wall -Wno-unused-function -pthread

all: genkey genmatrix secret checkmatrix

genkey: genkey.c matrix.c bignum.c ARC4.c random.c generators.c bmat.h generators.h
	gcc $(CFLAGS) -o genkey genkey.c matrix.c bignum.c ARC4.c random.c generators.c -lm

genmatrix: genmatrix.c matrix.c bignum.c ARC4.c random.c bmat.h
	gcc $(CFLAGS) -o genmatrix genmatrix.c matrix.c bignum.c ARC4.c random.c -lm

secret: secret.c matrix.c bignum.c ARC4.c random.c generators.c bmat.h generators.h
	gcc $(CFLAGS) -o secret secret.c matrix.c bignum.c ARC4.c random.c generators.c -lm

checkmatrix: checkmatrix.c matrix.c bignum.c ARC4.c random.c bmat.h
	gcc $(CFLAGS) -o checkmatrix checkmatrix.c matrix.c bignum.c ARC4.c random.c -lm
