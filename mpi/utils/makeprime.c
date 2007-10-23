/*
  makeprime.c

  A simple prime generator function (and test driver).  Prints out the
  first prime it finds greater than or equal to the starting value.
  
  Usage: makeprime <start>

  by Michael J. Fromberger <sting@linguist.dartmouth.edu>
  Copyright (C) 2000 Michael J. Fromberger, All Rights Reserved

  $Id: makeprime.c,v 1.1 2004/02/08 04:28:32 sting Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* These two must be included for make_prime() to work */

#include "mpi.h"
#include "mpprime.h"

/*
  make_prime(p, nr)

  Find the smallest prime integer greater than or equal to p, where
  primality is verified by 'nr' iterations of the Rabin-Miller
  probabilistic primality test.  The caller is responsible for
  generating the initial value of p.

  Returns MP_OKAY if a prime has been generated, otherwise the error
  code indicates some other problem.  The value of p is clobbered; the
  caller should keep a copy if the value is needed.  
 */
mp_err   is_prime(mp_int *p, int nr);
mp_err   make_prime(mp_int *p, int nr, int strong);

/* The main() is not required -- it's just a test driver */
int main(int argc, char *argv[])
{
  mp_int    start;
  mp_err    res;
  int       strong = 0;

  if(argc < 2) {
    fprintf(stderr, "Usage: %s <start-value> [strong]\n", argv[0]);
    return 1;
  }
	    
  mp_init(&start);
  if(argv[1][0] == '0' && tolower(argv[1][1]) == 'x') {
    mp_read_radix(&start, (unsigned char *)argv[1] + 2, 16);
  } else {
    mp_read_radix(&start, (unsigned char *)argv[1], 10);
  }
  mp_abs(&start, &start);

  if(argc > 2 && strcmp(argv[2], "strong") == 0) {
    strong = 1;
  }

  if((res = make_prime(&start, 5, strong)) != MP_OKAY) {
    fprintf(stderr, "%s: error: %s\n", argv[0], mp_strerror(res));
    mp_clear(&start);

    return 1;

  } else {
    unsigned char  *buf = malloc(mp_radix_size(&start, 10));

    mp_todecimal(&start, buf);
    printf("%s\n", (char *)buf);
    free(buf);
    
    mp_clear(&start);

    return 0;
  }
  
} /* end main() */

/*------------------------------------------------------------------------*/

mp_err   is_prime(mp_int *p, int nr)
{
  mp_digit   which = prime_tab_size;
  mp_err     res;

  if((res = mpp_divis_primes(p, &which)) == MP_YES)
    return MP_NO;
  else if(res != MP_NO)
    return res;

  if((res = mpp_fermat(p, 2)) == MP_NO)
    return MP_NO;
  else if(res != MP_YES)
    return res;

  return mpp_pprime(p, nr);

} /* end is_prime() */

mp_err   make_prime(mp_int *p, int nr, int strong)
{
  mp_err  res;

  if(mp_iseven(p)) {
    mp_add_d(p, 1, p);
  }

  do {
    if((res = is_prime(p, nr)) == MP_YES) {
      if(strong) {
	mp_int  p2;

	if((res = mp_init_copy(&p2, p)) != MP_OKAY)
	  return res;

	mp_sub_d(&p2, 1, &p2);
	mp_div_2(&p2, &p2);

	res = is_prime(&p2, nr);
	mp_clear(&p2);
	if(res == MP_YES)
	  break;
	else if(res != MP_NO)
	  return res;
	
      } else
	return MP_OKAY;
    } else if(res != MP_NO) {
      return res;
    }
 
  } while((res = mp_add_d(p, 2, p)) == MP_OKAY);

  return res;

} /* end make_prime() */

/*------------------------------------------------------------------------*/
/* HERE THERE BE DRAGONS                                                  */
