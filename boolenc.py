# This is an attempt at a Public Key Cryptosystem (PKC).  It's based on some
# cool XOR based arithmetic.

import random

class Matrix:
    """This is a simple binary matrix class."""

    def __init__(self, value=[[1]]):
        self.value = value
        self.rows = len(value)
        self.cols = len(value[0])

    def copy(self):
        value = [row[:] for row in self.value]
        return Matrix(value)
    
    def show(self, name=None):
        if name != None:
            print name
        for row in range(self.rows):
            string = ""
            for col in range(self.cols):
                if self.value[row][col]:
                    string += "1"
                else:
                    string += "0"
            print string
    
    def __add__(self, other):
        # check if correct dimensions
        if self.rows != other.rows or self.cols != other.cols:
            raise ValueError, "Matrices must be of equal dimensions to add"
        else:
            # add if correct dimensions
            res = Matrix.zero(self.rows, self.cols)
            for i in range(self.rows):
                for j in range(self.cols):
                    res.value[i][j] = self.value[i][j] ^ other.value[i][j]
            return res
    
    def __mul__(self, other):
        # check if correct dimensions
        if self.cols != other.rows:
            raise ValueError, "Matrices must be m*n and n*p to multiply"
        else:
            # multiply if correct dimensions
            res = Matrix.zero(self.rows, other.cols)
            for i in range(self.rows):
                for j in range(other.cols):
                    for k in range(self.cols):
                        res.value[i][j] ^= self.value[i][k] and other.value[k][j]
            return res

    def __getitem__(self, i):
        return self.value[i]

    def __setitem__(self, i, rowVal):
        self.value[i] = rowVal

    def transpose(self):
        # compute transpose
        res = Matrix.zero(self.cols, self.rows)
        for i in range(self.rows):
            for j in range(self.cols):
                res.value[j][i] = self.value[i][j]
        return res
    
    def inverse(self):
        I = Matrix.identity(self.rows)
        A = self.copy()
        # Start with A and I, and do Gaussian elimination to convert A to I,
        # while doing the same operations to the other matrix.
        # Do the row additions tcolo zero out lower left of A:
        for row in range(A.rows):
            lowerRow = A.findNonZeroRow(row)
            if lowerRow == -1:
                raise ValueError, "Matrix is singular"
            if lowerRow > row:
                A.xorRow(lowerRow, row)
                I.xorRow(lowerRow, row)
            for lowerRow in range(row + 1, A.rows):
                if A.value[lowerRow][row]:
                    A.xorRow(row, lowerRow)
                    I.xorRow(row, lowerRow)
        # Do the same thing to zero the upper part
        for row in range(A.rows-1, -1, -1):
            for upperRow in range(0, row):
                if A.value[upperRow][row]:
                    A.xorRow(row, upperRow)
                    I.xorRow(row, upperRow)
        return I

    def findNonZeroRow(self, pos):
        """Starts looking at row = pos, col = pos, and searches down."""
        for row in range(pos, self.rows):
            if self.value[row][pos]:
                return row
        return -1

    def xorRow(self, source, dest):
        """XOR the source row into the dest row."""
        for col in range(self.cols):
            self.value[dest][col] ^= self.value[source][col]

    def xorCol(self, source, dest):
        """XOR the source column into the dest column."""
        for row in range(self.rows):
            self.value[row][dest] ^= self.value[row][source]
    
    def __repr__(self):
        return repr(self.value)

    @staticmethod
    def zero(rows, cols):
        value = [[False for i in range(cols)] for j in range(rows)]
        return Matrix(value)

    @staticmethod
    def identity(size):
        matrix = Matrix.zero(size, size)
        for i in range(size):
            matrix.value[i][i] = 1
        return matrix

    @staticmethod
    def randomNonSingularMatrix(size):
        """Create a random non-singular Boolean matrix."""
        i = 0
        while True:
            m = Matrix.randomMatrix(size, size)
            i += 1
            if not m.isSingular():
                print "Generated signular matrix in", i, "tries"
                return m

    @staticmethod
    def randomMatrix(rows, cols):
        """Create a random Boolean matrix."""
        m = Matrix.zero(rows, cols)
        for row in range(rows):
            for col in range(cols):
                m[row][col] = random.randrange(2) == 1
        return m

    def isSingular(self):
        A = self.copy()
        for pos in range(A.cols):
            row = A.findNonZeroRow(pos) # Starts looking at row = pos, col = pos
            if row == -1:
                return True
            if row > pos:
                A.xorRow(row, pos)
            for row in range(pos + 1, A.rows):
                if A.value[row][pos]:
                    A.xorRow(pos, row)
        return False
    
    def expand(self, rowVal):
        """Add a zero column to the matrix, and then add row."""
        while len(rowVal) > self.cols:
            for row in range(self.rows):
                self.value[row].append(False)
            self.cols += 1
        if len(rowVal) != self.cols:
            raise ValueError, "expanding by too small row"
        self.value.append(rowVal)
        self.rows += 1

class Equ:
    """This class represents a class of Boolean equations written in XOR/AND
       cannonical form.  Only single and double cubes are supported."""
    def __init__(self, varName = None):
        self.singles = {}
        self.doubles = {}
        if varName != None:
            self.singles[varName] = varName

    def __add__(self, other):
        equ = Equ()
        for single in self.singles.keys():
            if not single in other.singles:
                equ.singles[single] = None
        for single in other.singles.keys():
            if not single in self.singles:
                equ.singles[single] = None
        for double in self.doubles.keys():
            if not double in other.doubles:
                equ.doubles[double] = None
        for double in other.doubles.keys():
            if not double in self.doubles:
                equ.doubles[double] = None
        return equ

    def show(self, name=None):
        """Print out an equation."""
        if name != None:
            print name
        if len(self.singles) + len(self.doubles) == 0:
            print "0"
            return
        singles = self.singles.keys()
        singles.sort()
        doubles = self.doubles.keys()
        doubles.sort()
        s = ""
        for cube in singles:
            s += " + " + cube
        for cube in doubles:
            s += " + " + cube
        print s[3:]

class XorEnc:
    """Public key cryptosystem based on xor based equations.
       We can represent this form of permutations with two Boolean matricies,
       one for the single variable cubes, and one for the doubles."""
    
    def __init__(self, bits):
        self.N = bits
        self.computeRandomKey()

    def computeRandomKey(self):
        """Generate a random linear permutation of size N/2.  Call it P, with
        inverse Q."""
        #Randomly pick n two variable cubes of X[:N/2], with no duplicates and
        #with both vars different.  Call this set R.
        N = self.N
        X = ['x' + str(i) for i in range(N)]
        R = self.pickRandomCubes()
        self.R = R
        self.Psingles = Matrix.randomNonSingularMatrix(N/2)
        self.Pdoubles = Matrix.zero(N/2, N)
        self.Qsingles = self.Psingles.inverse()
        self.Qdoubles = Matrix.zero(N/2, N)
        self.Psingles.show("P singles")
        self.Qsingles.show("Q singles")
        self.Pdoubles.show("P doubles")
        self.Qdoubles.show("Q doubles")
        (self.Qsingles*self.Psingles).show("Q*P singles")
        for i in range(N/2+1, N):
            self.expandPermByOne(i)

    def expandPermByOne(self, i):
        """i is the index of the new variable."""
        N = self.N
        Ps = self.Psingles
        Pd = self.Pdoubles
        # Compute F[i] = X[i] + randSel(X[0:i-1]) + randSel(R)
        Fs = randomSelection(i-1)
        Fs.append(True) # Add our own variable
        Fd = randomSelection(N)
        Ps.expand(Fs)
        Pd.expand(Fd)
        # Add G into F[0] .. F[i-1]
        for j in range(i-1):
            if random.randrange(2) == 1:
                self.addInFunction(Fs, Ps[j])
                self.addInFunction(Fd, Pd[j])
        # Also compute new Q
        Ps.show("Final Ps")
        Pd.show("Final Pd")

    @staticmethod
    def addInFunction(source, dest):
        """Boolean XOR in the first list into the second."""
        for i in range(len(source)):
            dest[i] ^= source[i]

    def pickRandomCubes(self):
        """Return a list of N random 2-variable cubes made from variables in X."""
        i = 0
        cubes = {}
        while i < self.N:
            cube = self.random2VarCube()
            if not cube in cubes:
                cubes[cube] = None
                i += 1
        return cubes.keys()

    def random2VarCube(self):
        """Return a random 2-variable cube from the lower half of variables.  Don't
        allow the same variable to be chosen twice."""
        v1 = random.randrange(self.N/2)
        v2 = v1
        while v2 == v1:
            v2 = random.randrange(self.N/2)
        if v1 <= v2:
            return (v1, v2)
        return (v2, v1)

def showRow(rowVal, label=""):
    s = label + " "
    for i in range(len(rowVal)):
        if rowVal[i]:
            s += "1"
        else:
            s += "0"
    print s

def randomSelection(N):
    """Return a list of numbers randomly chosen from 0 .. N-1."""
    l = []
    for i in range(N):
        if random.randrange(2) == 1:
            l.append(True)
        else:
            l.append(False)
    showRow(l, "random selection")
    return l


#m = Matrix([[True, False, False], [True, False, True], [False, True, True]])
#m.show()
#m.inverse().show()

enc = XorEnc(16)
