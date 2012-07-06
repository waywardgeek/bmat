genkey: genkey.c matrix.c bignum.c ARC4.c bmat.h
	gcc -Wall -Wno-unused -g -o genkey genkey.c matrix.c bignum.c ARC4.c
