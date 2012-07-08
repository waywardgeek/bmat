#CFLAGS=-g -Wall -Wno-unused
CFLAGS=-O2 -Wall -Wno-unused-function

all: genkey genmatrix secret

genkey: genkey.c matrix.c bignum.c ARC4.c random.c bmat.h
	gcc $(CFLAGS) -o genkey genkey.c matrix.c bignum.c ARC4.c random.c

genmatrix: genmatrix.c matrix.c bignum.c ARC4.c random.c bmat.h
	gcc $(CFLAGS) -o genmatrix genmatrix.c matrix.c bignum.c ARC4.c random.c

secret: secret.c matrix.c bignum.c ARC4.c random.c bmat.h
	gcc $(CFLAGS) -o secret secret.c matrix.c bignum.c ARC4.c random.c
