/*
  factor.c

  Large integer factorization.

  by Michael J. Fromberger <sting@linguist.dartmouth.edu>
  Copyright (C) 2001 Michael J. Fromberger, All Rights Reserved.

  $Id: mpfactor.c,v 1.1 2004/02/08 04:28:32 sting Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "mpi.h"
#include "mpprime.h"

#define NUM_TRIES 65536

int find_small_factor(mp_int *mp, mp_int *factor);
int find_big_factor(mp_int *mp, mp_digit base, int tries, mp_int *factor);

int main(int argc, char *argv[])
{
  int              len, tries = NUM_TRIES;
  unsigned char   *buf;
  mp_int           value, factor;

  if(argc < 2) {
    fprintf(stderr, "Usage: %s <value>\n", argv[0]);
    return 1;
  }

  mp_init(&value); mp_init(&factor);
  mp_read_radix(&value, (unsigned char *)argv[1], 10);

  if(argc > 2) {
    tries = abs(atoi(argv[2]));
    if(tries <= 0)
      tries = 1;
  }
  
  len = mp_radix_size(&value, 10);
  buf = malloc(len);

  while(find_small_factor(&value, &factor)) {
    mp_todecimal(&factor, buf);
    printf("%s\n", buf);

    mp_div(&value, &factor, &value, NULL);
  }

  while(find_big_factor(&value, 3, tries, &factor)) {
    mp_todecimal(&factor, buf);
    printf("%s\n", buf);

    mp_div(&value, &factor, &value, NULL);
  }

  mp_clear(&factor);
  mp_clear(&value);

  return 0;
}

int find_small_factor(mp_int *mp, mp_int *factor)
{
  mp_digit  np;

  np = prime_tab_size;
  if(mpp_divis_primes(mp, &np) == MP_YES) {
    mp_set(factor, np);
    return 1;
  } else {
    return 0;
  }
}

int find_big_factor(mp_int *mp, mp_digit base, int tries, mp_int *factor)
{
  mp_int   test, gcd, k;
  int      res = 0;

  mp_init(&test); mp_init(&gcd); mp_init(&k); 
  mp_set(&test, base);
  mp_set(&k, 1);

  while(--tries >= 0) {
    mp_exptmod(&test, &k, mp, &test);
    mp_sub_d(&test, 1, &test);
    mp_gcd(&test, mp, &gcd);

    /* See if we have found a nontrivial factor */
    if(mp_cmp_d(&gcd, 1) != 0 &&
       mp_cmp(&gcd, mp) != 0) {

      mp_copy(&gcd, factor);
      fprintf(stderr, "tries left = %d\n", tries);
      res = 1;
      break;
    } else {
      mp_add_d(&test, 1, &test);
      mp_add_d(&k, 1, &k);
    }
  }

  mp_clear(&k);
  mp_clear(&gcd);
  mp_clear(&test);

  return res;
}
