/* 
   metime.c

   by Michael J. Fromberger <sting@linguist.dartmouth.edu>
   Copyright (C) 2000 Michael J. Fromberger, All Rights Reserved.

   Modular exponentiation timing test

   $Id: metime.c,v 1.1 2004/02/08 04:28:32 sting Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include "mpi.h"
#include "mpprime.h"

double clk_to_sec(clock_t start, clock_t stop);

int main(int argc, char *argv[])
{
  int          ix, num, prec = 8;
  unsigned int seed;
  clock_t      start, stop;
  double       sec;

  mp_int     a, m, c;

  if(getenv("SEED") != NULL)
    seed = abs(atoi(getenv("SEED")));
  else 
    seed = (unsigned int)time(NULL);

  if(argc < 2) {
    fprintf(stderr, "Usage: %s <num-tests> [<nbits>]\n", argv[0]);
    return 1;
  }

  if((num = atoi(argv[1])) < 0)
    num = -num;

  if(!num) {
    fprintf(stderr, "%s: must perform at least 1 test\n", argv[0]);
    return 1;
  }

  if(argc > 2) {
    if((prec = atoi(argv[2])) <= 0)
      prec = 8;
    else 
      prec = (prec + (MP_DIGIT_BIT - 1)) / MP_DIGIT_BIT;

  }
  
  printf("Modular exponentiation timing test\n"
	 "Precision:  %d digits (%d bits)\n"
	 "# of tests: %d\n\n", prec, (int)(prec * MP_DIGIT_BIT), num);

  mp_init_size(&a, prec);
  mp_init_size(&m, prec);
  mp_init_size(&c, prec);

  srand(seed);

  start = clock();
  for(ix = 0; ix < num; ix++) {
    mpp_random_size(&a, prec);
    mpp_random_size(&c, prec);
    mpp_random_size(&m, prec);
    mp_exptmod(&a, &c, &m, &c);
  }
  stop = clock();

  sec = clk_to_sec(start, stop);

  printf("Total:      %.3f seconds\n", sec);
  printf("Individual: %.3f seconds\n", sec / num);

  mp_clear(&c);
  mp_clear(&a);
  mp_clear(&m);

  return 0;
}

double clk_to_sec(clock_t start, clock_t stop)
{
  return (double)(stop - start) / CLOCKS_PER_SEC;
}

/*------------------------------------------------------------------------*/
/* HERE THERE BE DRAGONS                                                  */
