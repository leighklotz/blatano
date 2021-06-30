#!/usr/bin/env python3

from random import choices, choice
from math import floor
from crc32c import crc32c

CONSONANTS=list("kgstnvhbpmyrlw")                             # 13
VOWELS=list("aeiou")                                          #  5
NR=list("nr")                                              #  5 [3 spaces at beg]
NXR=list("xnr")                                              #  4 [1 space at beg]
SUFFIXES=[ "tor", "tron", "on", "in", "ivor", "oron", "ist" ] #  7

NUM_SYLLABLES = len(CONSONANTS) * len(VOWELS)
S1_LEN = (NUM_SYLLABLES+1) * (len(NXR)+1)
S2_LEN = (NUM_SYLLABLES+1) * (len(NR)+3)
S3_LEN = S1_LEN
S4_LEN = len(SUFFIXES) + 5

def get_syllable(s):
    ci = s//len(VOWELS)
    if ci == len(CONSONANTS):
        result=''
    else:
        result = CONSONANTS[ci] + VOWELS[s%len(VOWELS)]
    return result

def get_s1_idx(nhash):
    #import pdb; pdb.set_trace()
    goal = floor(nhash % S1_LEN)
    print(f"- goal=0x{(goal & 0xffffffff):x}")
    s = goal % (NUM_SYLLABLES+1)
    print(f"- s=0x{(s & 0xffffffff):x}")
    n = goal // (NUM_SYLLABLES+1) - 1
    print(f"- gn=0x{(n & 0xffffffff):x}");

    print(f"- nhash=0x{(n&0xffffffff):x}, S1_LEN={S1_LEN}, goal={goal}, NUM_SYLLABLES={NUM_SYLLABLES}, n={n}")

    if n<0:
        return get_syllable(s)
    else:
        return get_syllable(s)+NXR[n]

def get_s2_idx(nhash):
    goal = floor(nhash % S2_LEN)
    s = goal % (NUM_SYLLABLES+1)
    n = goal // (NUM_SYLLABLES+1)
    n -= 3
    if n<0:
        return get_syllable(s)
    else:
        return get_syllable(s)+NR[n]

def get_s3_idx(nhash):
    return get_s1_idx(nhash)

def get_s4_idx(nhash):
    goal = floor(nhash % S4_LEN)
    if goal >= len(SUFFIXES):
        return ''
    else:
        return SUFFIXES[goal]

def main():
    if False:
        hex_test=[0x0, 0x1, 0x2,
                  0x10, 0x100, 0x1000, 0x10000,
                  0x20000, 0x20001,
                  0x7fffffff, 0x80000000, 0x80000001,
                  0xFFFFFFFE, 0xFFFFFFFF ]
        for h in hex_test:
            print(f"{hex(h)}: {robot_named_n(h)}")

    if False:
        assert(robot_named_n(0) == "voxhomuxist")
        assert(robot_named_n(1) == "numakutron")
        assert(robot_named_n(1000000) == "purwersen")

    if True:
        assert(robot_named_n(0x7ffffffe) == "sixwergur")

    if False:
        assert(robot_named_n(2**32-1) == "goxyokaist")

    if False:
        for n in range(0, 2**32, 2**28):
            print(f"{hex(n)}: {robot_named_n(n).capitalize()}; {hex(n+1)}: {robot_named_n(n+1).capitalize()}")

    if False:
        if (robot_named_n(0) != "voxhomuxist"):
            print("WARNING: NONSTANDARD MAPPING DETECTED: robot_0 = {robot_named_n(0)}")
        if (robot_named_n(1) != "numakutron"):
            print("WARNING: NONSTANDARD MAPPING DETECTED: robot_1 = {robot_named_n(1)}")

def robot_named_n(n):
    nhash=crc32c(n.to_bytes(4, 'big'))
    print(f"robot_named_n n=0x{n:x} nhash=0x{nhash:x}")
    l = []
    l.append(get_s1_idx(nhash)); nhash //= S1_LEN
    l.append(get_s2_idx(nhash)); nhash //= S2_LEN
    l.append(get_s3_idx(nhash)); nhash //= S3_LEN
    l.append(get_s4_idx(nhash)); nhash //= S4_LEN
        


    s = "".join(l)
    return(s)

if __name__=="__main__":
    main()
