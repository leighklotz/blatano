#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char *CONSONANTS="kgstnvhbpmyrlw";
const char *VOWELS="aeiou";
const char *NR="nr";		// 5 [3 spaces at beg]
const char *NXR="xnr";		//  4 [1 space at beg]
const char *SUFFIXES[]={ "tor", "tron", "on", "in", "ivor", "oron", "ist" }; //  7

int VOWELS_LEN;
int CONSONANTS_LEN;
int SUFFIXES_LEN;
int NUM_SYLLABLES;
int S1_LEN;
int S2_LEN;
int S3_LEN;
int S4_LEN;

void setup_crc() {
  Serial.println("setup_crc");
  VOWELS_LEN=strlen(VOWELS);
  CONSONANTS_LEN=strlen(CONSONANTS);
  SUFFIXES_LEN = sizeof(SUFFIXES) / sizeof(SUFFIXES[0]);
  NUM_SYLLABLES = strlen(CONSONANTS) * strlen(VOWELS);

  S1_LEN = (NUM_SYLLABLES+1) * (strlen(NXR)+1);
  S2_LEN = (NUM_SYLLABLES+1) * (strlen(NR)+3);
  S3_LEN = S1_LEN;
  S4_LEN = (sizeof(SUFFIXES) / sizeof(char *)) + 5;
  Serial.println("setup_crc_done");
}

char *add_syllable(char *buf, uint32_t s) {
  char *bbuf = buf;
  uint32_t ci = s/VOWELS_LEN;
  if (ci == CONSONANTS_LEN) {
    return buf;
  }
  (*bbuf++) = CONSONANTS[ci];
  (*bbuf++) = VOWELS[s % VOWELS_LEN];
  return bbuf;
}

char *add_s1(char *buf, uint32_t nhash) {
  char *bbuf = buf;
  uint32_t goal = nhash % S1_LEN;
  // printf("- goal=0x%x\n", goal);
  uint32_t s = goal % (NUM_SYLLABLES+1);
  // printf("- s=0x%x\n", s);
  uint32_t n = (goal / (NUM_SYLLABLES+1));
  // printf("- n=0x%x\n", n);
  // printf("- nhash=0x%x, S1_LEN=%d, goal=0x%X, NUM_SYLLABLES=%u, n=%u\n", nhash, S1_LEN, goal, NUM_SYLLABLES, n);
  bbuf = add_syllable(bbuf, s);
  if (n != 0) {
    (*bbuf++) = NXR[n-1];
  }
  return bbuf;
}

char *add_s2(char *buf, uint32_t nhash) {
  char *bbuf = buf;
  int goal = nhash % S2_LEN;
  int s = goal % (NUM_SYLLABLES+1);
  int n = (goal / (NUM_SYLLABLES+1)) - 3;
  bbuf = add_syllable(bbuf, s);
  if (n>=0) {
    (*bbuf++) = NR[n];
  }
  return bbuf;
}

char *add_s3(char *buf, uint32_t nhash) {
  return add_s1(buf, nhash);
}

char *add_s4(char *buf, uint32_t nhash) {
  int goal = nhash % S4_LEN;
  if (goal >= SUFFIXES_LEN) {
    // printf("s4 goal = %d SUFFIXES_LEN= %d nhash=0x%X S4_LEN=%d\n", goal, SUFFIXES_LEN, nhash, S4_LEN);
    return buf;
  } else {
    const char *s = SUFFIXES[goal];
    strcpy(buf, s);		// hope there's room in buf
    return buf + strlen(s);
  }
}

// https://inbox.dpdk.org/dev/56BC4481.1060009@6wind.com/T/
uint32_t crc32c_trivial(uint8_t *buffer, uint32_t length, uint32_t crc)
{
    uint32_t i, j;
    for (i = 0; i < length; ++i) {
        crc = crc ^ buffer[i];
        for (j = 0; j < 8; j++)
            crc = (crc >> 1) ^ 0x80000000 ^ ((~crc & 1) * 0x82f63b78);
    }
    return crc;
}

uint32_t get_crc32c(uint32_t n) {
  uint8_t a[4];
  uint32_t nhash = 0;
  a[0] = n >> 24;
  a[1] = n >> 16;
  a[2] = n >>  8;
  a[3] = n;

  nhash = crc32c_trivial(a, 4, 0);

  // PRINTLNI("- 0x%x hash 0x%x\n", n, nhash);
  return nhash;
}

char *robot_named_n(char *buf, uint32_t nhash) {
  // char *robot_named_n(char *buf, uint32_t n)
  //  uint32_t nhash = get_crc32c(n);
  // PRINTLNI("\nrobot named 0x%x: \n", n);
  char *bbuf = buf;
 
  bbuf = add_s1(bbuf, nhash); nhash /= S1_LEN;
  bbuf = add_s2(bbuf, nhash); nhash /= S2_LEN;
  bbuf = add_s3(bbuf, nhash); nhash /= S3_LEN;
  bbuf = add_s4(bbuf, nhash); nhash /= S4_LEN;
  *bbuf = '\0';
  // PRINTLNIS("robot named 0x%x is %s\n", n, buf);
  return buf;
}

#if TEST
void assert_equal(uint32_t n, char *expected, char *got) {
  int ok = (! strcmp(expected, got));
  const char *status = ok ? "OK" : "BAD";
  if (! ok || true) {
    printf("[%s] 0x%x expected %s got %s\n", status, n, expected, got);
  }
}

void test_crc_one(uint32_t n, uint32_t expected) {
  uint32_t nhash = get_crc32c(n);
  if (nhash == expected)  {
    printf("[OK] 0x%x hash 0x%x 0x%x\n", n, nhash, expected);
  } else {
    printf("[BAD] 0x%x hash 0x%x not 0x%x\n", n, nhash, expected);
  }
}

void test_crc() {
  Serial.print("test_crc");
  test_crc_one(0, 0x48674bc7);
  test_crc_one(0xFFFFFFFF, 0xFFFFFFFF);
  test_crc_one(0xFFFFFFFE, 0x0D947CFC);
  test_crc_one(0xFFFEFFFF, 0x5abe6d81);
  Serial.print("test_crc_done");
}

void test_names() {
  Serial.print("test_names");
  char buf[64];
  uint32_t big = 0xffffffff;
  assert_equal(0, "voxhomuxist", robot_named_n(buf, get_crc32c(0)));
  assert_equal(1, "numakutron", robot_named_n(buf, get_crc32c(1)));
  assert_equal(1000000, "purwersen", robot_named_n(buf, get_crc32c(1000000)));
  assert_equal(0x7ffffffe, "sixwergur", robot_named_n(buf, get_crc32c(0x7ffffffe)));
  assert_equal(0x7fffffff, "yoxkonunoron", robot_named_n(buf, get_crc32c(0x7fffffff)));
  assert_equal(0x80000000, "linhayunin", robot_named_n(buf, get_crc32c(0x80000000)));
  assert_equal(0x80000001, "werlinoxivor", robot_named_n(buf, get_crc32c(0x80000001)));
  assert_equal(big-1, "bixgirror", robot_named_n(buf, get_crc32c(big-1)));
  assert_equal(big, "goxyokaist", robot_named_n(buf, get_crc32c(big)));
  Serial.print("test_names_done");
}

#endif
