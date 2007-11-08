/*
  Simple test driver for MPI library

  Test 3: Multiplication, division, and exponentiation test

  $Id: mptest-3.c,v 1.1 2004/02/08 04:28:59 sting Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <time.h>

#include "mpi.h"

#define  SQRT 0  /* define nonzero to get square-root test  */
#define  EXPT 0  /* define nonzero to get exponentiate test */

int main(int argc, char *argv[])
{
  int      ix;
  mp_int   a, b, c, d;
  mp_digit r;
  mp_err   res;

  if(argc < 3) {
    fprintf(stderr, "Usage: %s <a> <b>\n", argv[0]);
    return 1;
  }

  printf("Test 3: Multiplication and division\n\n");
  srand(time(NULL));

  mp_init(&a);
  mp_init(&b);

  mp_read_radix(&a, argv[1], 10);
  mp_read_radix(&b, argv[2], 10);
  printf("a = "); mp_print(&a, stdout); fputc('\n', stdout);
  printf("b = "); mp_print(&b, stdout); fputc('\n', stdout);
  
  mp_init(&c);
  printf("\nc = a * b\n");

  mp_mul(&a, &b, &c);
  printf("c = "); mp_print(&c, stdout); fputc('\n', stdout);

  printf("\nc = b * 32523\n");

  mp_mul_d(&b, 32523, &c);
  printf("c = "); mp_print(&c, stdout); fputc('\n', stdout);
  
  mp_init(&d);
  printf("\nc = a / b, d = a mod b\n");
  
  mp_div(&a, &b, &c, &d);
  printf("c = "); mp_print(&c, stdout); fputc('\n', stdout);  
  printf("d = "); mp_print(&d, stdout); fputc('\n', stdout);  

  ix = rand() % 256;
  printf("\nc = a / %d, r = a mod %d\n", ix, ix);
  mp_div_d(&a, (mp_digit)ix, &c, &r);
  printf("c = "); mp_print(&c, stdout); fputc('\n', stdout);  
  printf("r = %04X\n", r);

#if EXPT
  printf("\nc = a ** b\n");
  mp_expt(&a, &b, &c);
  printf("c = "); mp_print(&c, stdout); fputc('\n', stdout);  
#endif

  ix = rand() % 256;
  printf("\nc = 2^%d\n", ix);
  mp_2expt(&c, ix);
  printf("c = "); mp_print(&c, stdout); fputc('\n', stdout);

#if SQRT
  printf("\nc = sqrt(a)\n");
  if((res = mp_sqrt(&a, &c)) != MP_OKAY) {
    printf("mp_sqrt: %s\n", mp_strerror(res));
  } else {
    printf("c = "); mp_print(&c, stdout); fputc('\n', stdout);
    mp_sqr(&c, &c);
    printf("c^2 = "); mp_print(&c, stdout); fputc('\n', stdout);
  }
#endif

  mp_clear(&d);
  mp_clear(&c);
  mp_clear(&b);
  mp_clear(&a);

  return 0;
}
