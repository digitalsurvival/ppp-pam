/*
  Simple test driver for MPI library

  Test 8: Probabilistic primality tester
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

  if(argc < 2) {
    fprintf(stderr, "Usage: %s <a>\n", argv[0]);
    return 1;
  }

  printf("Test 8: Probabilistic primality testing\n\n");

  mp_init(&a);

  mp_read_radix(&a, argv[1], 10);

  printf("a = "); mp_print(&a, stdout); fputc('\n', stdout);

  printf("\nChecking for divisibility by small primes ... \n");
  if(mpp_divis_primes(&a, 170) == MP_YES) {
    printf("it is not prime\n");
    goto CLEANUP;
  }
  printf("Passed that test (not divisible by any small primes).\n");

  for(ix = 0; ix < 10; ix++) {
    printf("\nPerforming Rabin-Miller test, iteration %d\n", ix + 1);

    if(mpp_pprime(&a) == MP_NO) {
      printf("it is not prime\n");
      goto CLEANUP;
    }
  }
  printf("All tests passed; a is probably prime\n");

CLEANUP:
  mp_clear(&a);

  return 0;
}
