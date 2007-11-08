/*
  primegen.c
	
  by Michael J. Fromberger <sting@linguist.dartmouth.edu>
  Copyright (C) 1998 Michael J. Fromberger, All Rights Reserved
	
  Generates random integers which are prime with a high degree of
  probability using the Miller-Rabin probabilistic primality testing
  algorithm.

  Usage:
     primegen <bits> [<num>]

     <bits>   - number of significant bits each prime should have
     <num>    - number of primes to generate

  $Id: primegen.c,v 1.1 2004/02/08 04:28:32 sting Exp $
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include "mpi.h"
#include "mpprime.h"

#undef MACOS		/* define if running on a Macintosh */

#ifdef MACOS
#include <console.h>
#endif

#define NUM_TESTS 5  /* Number of Rabin-Miller iterations to test with */

int    g_strong = 0;

int main(int argc, char *argv[])
{
  unsigned char *raw, *out;
  int		rawlen, bits, outlen, ngen, ix, jx;
  mp_int	testval, q, ntries;
  mp_err	res;
  mp_digit      np;
  clock_t	start, end;

#ifdef MACOS
  argc = ccommand(&argv);
#endif

  /* We'll just use the C library's rand() for now, although this
     won't be good enough for cryptographic purposes */

  if((out = (unsigned char *)getenv("SEED")) == NULL) {
    srand((unsigned int)time(NULL));
  } else {
    srand((unsigned int)atoi((char *)out));
  }

  if(argc < 2) {
    fprintf(stderr, "Usage: %s <bits> [<count> [strong]]\n", argv[0]);
    return 1;
  }
	
  if((bits = abs(atoi(argv[1]))) < CHAR_BIT) {
    fprintf(stderr, "%s: please request at least %d bits.\n",
	    argv[0], CHAR_BIT);
    return 1;
  }

  /* If optional third argument is given, use that as the number of
     primes to generate; otherwise generate one prime only.
   */
  if(argc < 3) {
    ngen = 1;
  } else {
    ngen = abs(atoi(argv[2]));
  }

  /* If fourth argument is given, and is the word "strong", we'll 
     generate strong (Sophie Germain) primes. 
   */
  if(argc > 3 && strcmp(argv[3], "strong") == 0)
    g_strong = 1;

  /* testval - candidate being tested
     ntries  - number tried so far 
     q       - used in finding strong primes 
   */
  if((res = mp_init(&testval)) != MP_OKAY ||
     (res = mp_init(&ntries)) != MP_OKAY ||
     (res = mp_init(&q)) != MP_OKAY) {
    fprintf(stderr, "%s: error: %s\n", argv[0], mp_strerror(res));
    return 1;
  }
  
  if(g_strong) {
    printf("Requested %d strong prime values of at least %d bits.\n", 
	   ngen, bits);
  } else {
    printf("Requested %d prime values of at least %d bits.\n", ngen, bits);
  }

  rawlen = (bits / CHAR_BIT) + ((bits % CHAR_BIT) ? 1 : 0);

  if((raw = calloc(rawlen, sizeof(unsigned char))) == NULL) {
    fprintf(stderr, "%s: out of memory, sorry.\n", argv[0]);
    return 1;
  }

  /* This loop is one for each prime we need to generate */
  for(jx = 0; jx < ngen; jx++) {

    /*	Pack the initializer with random bytes	*/
    for(ix = 0; ix < rawlen; ix++) 
      raw[ix] = (rand() * rand()) & UCHAR_MAX;

    raw[0] |= 0x80;             /* set high-order bit of test value     */
    if(g_strong)
      raw[rawlen - 1] |= 7;     /* set low-order 3 bits of test value   */
    else
      raw[rawlen - 1] |= 1;     /* set low-order bit of test value      */

    /* Make an mp_int out of the initializer */
    mp_read_unsigned_bin(&testval, raw, rawlen);

    /* If we asked for a strong prime, shift down one bit so that when
       we double, we're still within the right range of bits ... this
       is why we OR'd with 7 instead of 1 above (leaves two bits set 
       so that the value remains congruent to 3 (mod 4)).
     */
    if(g_strong) {
      mp_copy(&testval, &q);
      mp_div_2(&testval, &testval);
    }      

    /* Initialize candidate counter */
    mp_zero(&ntries);
    mp_add_d(&ntries, 1, &ntries);

    start = clock(); /* time generation for this prime */
    for(;;) {
      /*
	Test for divisibility by small primes (of which there is a table 
	conveniently stored in mpprime.c)
      */
      np = prime_tab_size;

      if(mpp_divis_primes(&testval, &np) == MP_NO) {

	/* If we're trying for a strong prime, test 2p+1 before
	   running other primality tests */
	if(g_strong) {
	  np = prime_tab_size;
	  if(mpp_divis_primes(&q, &np) == MP_YES)
	    goto NEXT_CANDIDATE;
	}

	/* If that passed, run a Fermat test */
	res = mpp_fermat(&testval, 2);
	switch(res) {
	case MP_NO:     /* composite        */
	  goto NEXT_CANDIDATE;
	case MP_YES:    /* may be prime     */
	  break;
	default:
	  goto CLEANUP; /* some other error */
	}
	
	/* If that passed, run some Miller-Rabin tests	*/
	res = mpp_pprime(&testval, NUM_TESTS);
	switch(res) {
	case MP_NO:     /* composite        */
	  goto NEXT_CANDIDATE;
	case MP_YES:    /* may be prime     */
	  break;
	default:
	  goto CLEANUP; /* some other error */
	}

	/* At this point, we have strong evidence that our candidate
	   is itself prime.  If we want a strong prime, we need now
	   to test q = 2p + 1 for primality...
	 */
 	if(g_strong) {
	  if(res == MP_YES) {
	    fputc('.', stderr);

	    /* If we get here, we've already tested q against small
	       prime divisors, so we can just do the regular primality
	       testing
	     */

	    /* Fermat, as with its parent ... */
	    res = mpp_fermat(&q, 2);
	    switch(res) {
	    case MP_NO:     /* composite        */
	      goto NEXT_CANDIDATE;
	    case MP_YES:    /* may be prime     */
	      break;
	    default:
	      goto CLEANUP; /* some other error */
	    }

	    /* And, with Miller-Rabin, as with its parent ... */
	    res = mpp_pprime(&q, NUM_TESTS);
	    switch(res) {
	    case MP_NO:     /* composite        */
	      goto NEXT_CANDIDATE;
	    case MP_YES:    /* may be prime     */
	      break;
	    default:
	      goto CLEANUP; /* some other error */
	    }
	  
	    /* If it passed, we've got a winner */
	    if(res == MP_YES) {
	      fputc('\n', stderr);
	      mp_copy(&q, &testval);
	      break;
	    }

	  } /* end if(res == MP_YES) */

	} else {
	  /* We get here if g_strong is false */
	  if(res == MP_YES)
	    break;
	}
      } /* end if(not divisible by small primes) */
      
      /*
	If we're testing strong primes, skip to the next odd value
	congruent to 3 (mod 4).  Otherwise, just skip to the next odd
	value
       */
    NEXT_CANDIDATE:
      if(g_strong) {
	mp_add_d(&q, 4, &q);
	mp_div_2(&q, &testval);
      } else
	mp_add_d(&testval, 2, &testval);
      mp_add_d(&ntries, 1, &ntries);
    } /* end of loop to generate a single prime */
    end = clock();
    
    printf("After %d tests, the following value is still probably prime:\n",
	   NUM_TESTS);
    outlen = mp_radix_size(&testval, 10);
    out = calloc(outlen, sizeof(unsigned char));
    mp_toradix(&testval, out, 10);
    printf("10: %s\n", (char *)out);
    mp_toradix(&testval, out, 16);
    printf("16: %s\n\n", (char *)out);
    free(out);
    
    printf("Number of candidates tried: ");
    outlen = mp_radix_size(&ntries, 10);
    out = calloc(outlen, sizeof(unsigned char));
    mp_toradix(&ntries, out, 10);
    printf("%s\n", (char *)out);
    free(out);

    printf("This computation took %ld clock ticks (%.2f seconds)\n",
	   (end - start), ((double)(end - start) / CLOCKS_PER_SEC));
    
    fputc('\n', stdout);
  } /* end of loop to generate all requested primes */
  
 CLEANUP:
  if(res != MP_OKAY) 
    fprintf(stderr, "%s: error: %s\n", argv[0], mp_strerror(res));

  free(raw);
  mp_clear(&testval); mp_clear(&q); mp_clear(&ntries);
  
  return 0;
}
