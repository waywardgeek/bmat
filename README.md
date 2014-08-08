Here's a key exchange scheme I've come up with.

Let G be an NxN random "Boolean matrix".  For example, we could choose G =

        1 1 0 0 
        1 0 0 1 
        0 1 1 1 
        1 1 0 1

In this case N = 4.  A required property of G is that no two rows be equal to
the XOR-sum of any other rows.  For example, this matrix does not work:

        1 1 0
        0 1 1
        1 0 1

because the first row XOR-ed with the second row equals the third row.  Over
1/5th of all random Boolean matrices had the required property, which is called
being "non-singular".  Whatever.

We define "matrix multiplication" of AxB in a manner similar to normal matrix
multiplication.  To compute the result in the i-th row, and j-th column, you
take the i-th row of A and multiply it with the j-th column of B.  It looks
like:

        res[i][j] = A[i][0]*B[0][j] + A[i][1]*B[1][j] + ... + A[i][N-1]*B[N-1][j]

Here, I use * to mean the AND operation and + to mean XOR.  Also, you can
multiply a matrix times a vertical vector, and it usually looks like:

        1   1 0 1   1
        0 = 1 1 0 * 1
        1   0 1 0   0

In my head, I take the vector on the right, rotate it 90 degrees
counter-clockwise, and AND it on the top row of the matrix.  This results in 1 0
0.  I then XOR these together to get 1, which I fill in the top row on the left.
Similarly I multiply the vector on the right times the middle row of the matrix
to get the 0 in the middle on the left.

Now that we have these operations, I can define G^m = G*G*G*G... m times.  It
turns out that if we choose G correctly, then G^m will not be equal to G unless
m == 2^N:

            G^m == G implies m = 2^N

So, for example, if N is 127, then we can keep multiplying m by G over and over
for 2^127 - 1 times before the result is converted back into m.  Let's also
define a simple vertical N-bit vector called O (for One), that has 1 at the
top and the rest 0.  For example O could be:

        1
        0
        0
        0

Here's the key exchange algorithm...

- We start with a publicly known "good" NxN random matrix, called G
- Alice picks a secret random number m between 1 and 2^N - 1.
- Bob picks a secret random number n between 1 and 2^N - 1.
- Alice computes (G^m)*O (This is an N-bit vertical vector).  She transmits this
  publicly to Bob.
- Bob computes (G^n)*O and transmits it to Alice.
- Alice reconstructs Bob's entire matrix G^n from the first row published by
  Bob.  Then, she raises it to the power of m to get (G^n)^m - G^(m*n). 
- Bob reconstructs Alice's entire matrix G^m from the first row published by
  Alice.  Then, she raises it to the power of n to get (G^n)^m - G^(m*n). 
- Bob and Alice use the first row of G^(m*n) as their shared secret.

They then use this key to talk using AES or some other shared-private-key algorithm.

Reconstruction of the entire matrix is pretty fast.  The slow part is actually
computing the power afterwards.  Alice reconstructs Bob's entire matrix H=G^n by
noting that H*G^k = G^k*H for all k.  We have he first row of H, called h, and
the entire matrix G^k, including it's first row gk.  We use the following to
create linear equations restricting the values in H:

        h*G^k == gk*H

The left side we compute directly.  The right side creates N constraits on
values of H.  We do this N times, for k=1, 2, ..., N.  Then we just solve the
linear system of equations for H.

This can also be used for in public key encryption, where anyone can send
encrypted documents to Bob anonymously, and only Bob can decrypt them.  However,
it does not seem to offer a simple way for Bob to sign documents.  To send an
encrypted document to Bob, Alice just attaches here public key (G^m)*O to a
document encrypted with the shared key.  Only Bob can decrypt it.

Now for the last trick... here's my conjecture as how to find good matrices G.
First, only use N if it is in this sequence:

2, 3, 5, 7, 13, 17, 19, 31, 61, 89, 107, 127, 521, 607, 1279, 2203, 2281, 3217, 4253

These are the exponents of Mersenne primes, which are primes of the form 2^N -
1.  Second, only accept non-singular random matrices with no eigenvectors,
meaning G + I is also non-singular.  Finally, require that the sequence G, G^2,
G^4, ... G(2^(N-1)) have unique values and that G^(2^N) == G.

The checkmatrix tool verified that up to N=40, the if N is not a Mersenne prime
exponent, then there are matrices that pass all the other tests, but are not
primitive generators of groups of order 2^N - 1.  No such matrix has been found
by the tool for any Mersenne prime exponent.  Many thousands of generators were
found for 31 and lower with no counter examples.
