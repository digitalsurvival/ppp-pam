/*
  Simple test driver for MPI library

  Test 7: Random and divisibility tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>

#define MP_IOFUNC
#include "mpi.h"

#include "mpprime.h"

int main(int argc, char *argv[])
{
  int       ix, size;
  mp_int    a, b;

  srand(time(NULL));

  if(argc < 3) {
    fprintf(stderr, "Usage: %s <a> <b>\n", argv[0]);
    return 1;
  }

  printf("Test 7: Random & divisibility tests\n\n");

  mp_init(&a);
  mp_init(&b);

  mp_read_radix(&a, argv[1], 10);
  mp_read_radix(&b, argv[2], 10);

  printf("a = "); mp_print(&a, stdout); fputc('\n', stdout);
  printf("b = "); mp_print(&b, stdout); fputc('\n', stdout);

  if(mpp_divis(&a, &b) == MP_YES)
    printf("a is divisible by b\n");
  else
    printf("a is not divisible by b\n");

  if(mpp_divis(&b, &a) == MP_YES)
    printf("b is divisible by a\n");
  else
    printf("b is not divisible by a\n");

  printf("\nb = mpp_random()\n");
  mpp_random(&b);
  printf("b = "); mp_print(&b, stdout); fputc('\n', stdout);
  mpp_random(&b);
  printf("b = "); mp_print(&b, stdout); fputc('\n', stdout);
  mpp_random(&b);
  printf("b = "); mp_print(&b, stdout); fputc('\n', stdout);

  printf("\nTesting a for divisibility by first 170 primes\n");
  if(mpp_divis_primes(&a, 170) == MP_YES)
    printf("It is divisible by at least one of them\n");
  else
    printf("It is not divisible by any of them\n");

  mp_clear(&b);
  mp_clear(&a);

  return 0;
}
