matrix: matrix.c ARC4.c boolenc.h
	gcc -Wall -Wno-unused -g -o matrix matrix.c ARC4.c
	#gcc -Wall -Wno-unused -O2 -o matrix matrix.c ARC4.c
