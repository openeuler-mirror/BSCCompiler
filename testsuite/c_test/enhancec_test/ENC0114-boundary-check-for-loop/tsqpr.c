/*
 * (c) Copyright 2015-2020 by Solid Sands B.V.,
 *     Amsterdam, the Netherlands. All rights reserved.
 *     Subject to conditions in the RESTRICTIONS file.
 * (c) Copyright 2011 ACE Associated Compiler Experts bv
 * (c) Copyright 2011 ACE Associated Computer Experts bv
 * All rights reserved.  Subject to conditions in RESTRICTIONS file.
 */

#include "def.h"

#define N 1024

char flags[N];

/*
 * Primes are sieved by eliminating multiples of numbers > 1
 * Squares are sieved by counting divisors modulo 2 (squares have an odd
 * number of divisors
 * The loop is subject to peeling and shows iteration space dependences.
 */
void
compute(void)
{
  int i;
  int j;

  for (i = 0; i < N; i++) {
    if (i == 0) {
      flags[i] = 7;
    } else {
    for (j = i; j < N; j += i) {
      if (j < 2 || i > 1 && j > i) {
        flags[j] |= 2;
      }
      flags[j] = (flags[j] ^ 1)
          | 4 * (j <= 1 || i >= 2 && j > i);
      }
    }
  }
}

MAIN
{
  int i;
  int j;
  int s;
  int ns;
  int ls;
  ls=-1;

  CVAL_HEADER("Some non-trivial loop control");

  compute();

  s = 0;
  ns = 0;
  for (i = 0; i < N; i++) {
    CVAL_CASE(i);
    if (flags[i] & 1) {
      /* should be square */
      CVAL_VERIFY(i == s);
      CVAL_VERIFY(ns * ns == i);
      /* compute next square */
      ls = s;
      s += 2 * ns++ + 1;
    }
    if (! (flags[i] & 2)) {
      /* should be prime, so not divisible
       * by 2 and odd integers > 2
       */
      for (j = 2; j < ls; j += (j == 2) + 2 * (j > 2)) {
        CVAL_VERIFY(i == 2 || i % j != 0);
      }
    }

    CVAL_VERIFY(((flags[i] & 2) != 0) == ((flags[i] & 4) != 0));
  }
}
