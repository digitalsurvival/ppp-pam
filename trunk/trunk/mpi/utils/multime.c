/*
  Test the speed of multiplication
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

#include "mpi.h"
#include "mpprime.h"

int main(int argc, char *argv[])
{
  int           ntests, prec, ix;
  unsigned int  seed;
  char         *senv;
  clock_t       start, stop;
  double        multime;
  mp_int        a, b, c;

  if((senv = getenv("SEED")) != NULL)
    seed = atoi(senv);
  else
    seed = (unsigned int)time(NULL);

  if(argc < 3) {
    fprintf(stderr, "Usage: %s <ntests> <nbits>\n", argv[0]);
    return 1;
  }

  if((ntests = abs(atoi(argv[1]))) == 0) {
    fprintf(stderr, "%s: must request at least 1 test.\n", argv[0]);
    return 1;
  }
  if((prec = abs(atoi(argv[2]))) < CHAR_BIT) {
    fprintf(stderr, "%s: must request at least %d bits.\n", argv[0],
	    CHAR_BIT);
    return 1;
  }

  prec = (prec + (DIGIT_BIT - 1)) / DIGIT_BIT;

  mp_init_size(&a, prec);
  mp_init_size(&b, prec);
  mp_init_size(&c, 2 * prec);

  srand(seed);
  start = clock();
  for(ix = 0; ix < ntests; ix++) {
    mpp_random_size(&a, prec);
    mpp_random_size(&b, prec);
    mp_mul(&a, &a, &c);
  }
  stop = clock();

  multime = (double)(stop - start) / CLOCKS_PER_SEC;

  printf("Total: %.4f\n", multime);
  printf("Individual: %.4f\n", multime / ntests);

  mp_clear(&a); mp_clear(&b); mp_clear(&c);
  return 0;

}
