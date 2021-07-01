#!/usr/bin/env python3

import re

N=0x17F12F
#N=0xFFFFFF
nn='{0:024b}'.format(N)
A=list('{0:08b}'.format((N & 0xFF0000) >> 16))+['0']
B=list('{0:08b}'.format((N & 0x00FF00) >> 8))+['0']
C=list('{0:08b}'.format((N & 0x0000FF) >> 0))+['0']
X=8

def row(*args):
    left_half, right_half = args[:4], args[4:]
    # return "".join(str(n) for n in left_half)
    return "".join(str(n) for n in args)

f = [
    row( A[X], A[X], A[X], A[X], A[X], A[X], A[X]),
    row( A[X], A[0], A[1], A[2], A[2], A[1], A[0]),
    row( A[X], A[3], A[4], A[5], A[5], A[4], A[3]),
    row( A[X], A[X], A[6], A[7], A[7], A[6], A[X]),
    row( B[X], B[X], B[X], B[0], B[0], B[X], B[X]),
    row( B[X], B[1], B[2], B[3], B[2], B[1], B[X]),
    row( B[X], B[4], B[5], B[6], B[4], B[3], B[X]),
    row( B[X], B[X], B[X], B[7], B[7], B[X], B[X]),
    row( C[X], C[X], C[X], C[0], C[0], C[X], C[X]),
    row( C[X], C[1], C[2], C[3], C[2], C[1], C[X]),
    row( C[4], C[5], C[6], C[7], C[7], C[6], C[4])
]

for line in f:
    print(line)

if False:
    for line in f:
        # replace everything until first 1 with 0
        newline = re.sub(r"")
        print(newline)
