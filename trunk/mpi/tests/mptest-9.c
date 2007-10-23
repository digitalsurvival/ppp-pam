/*
    mptest-9.c

    by Michael J. Fromberger <sting@linguist.dartmouth.edu>
    Copyright (C) 1998 Michael J. Fromberger, All Rights Reserved

    Test logical functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>

#include "mpi.h"
#include "mplogic.h"

int main(int argc, char *argv[])
{
  mp_int    a, b, c;
  int       pco;
  mp_err    res;

  printf("Test 9: Logical functions\n\n");

  if(argc < 3) {
    fprintf(stderr, "Usage: %s <a> <b>\n", argv[0]);
    return 1;
  }

  mp_init(&a); mp_init(&b); mp_init(&c);
  mp_read_radix(&a, argv[1], 10);
  mp_read_radix(&b, argv[2], 10);

  printf("a = "); mp_print(&a, stdout); fputc('\n', stdout);
  printf("b = "); mp_print(&b, stdout); fputc('\n', stdout);

  mpl_not(&a, &c);
  printf("~a = "); mp_print(&c, stdout); fputc('\n', stdout);

  mpl_and(&a, &b, &c);
  printf("a & b = "); mp_print(&c, stdout); fputc('\n', stdout);

  mpl_or(&a, &b, &c);
  printf("a | b = "); mp_print(&c, stdout); fputc('\n', stdout);

  mpl_xor(&a, &b, &c);
  printf("a ^ b = "); mp_print(&c, stdout); fputc('\n', stdout);

  mpl_rsh(&a, &c, 1);
  printf("a >> 1 = "); mp_print(&c, stdout); fputc('\n', stdout);
  mpl_rsh(&a, &c, 5);
  printf("a >> 5 = "); mp_print(&c, stdout); fputc('\n', stdout);
  mpl_rsh(&a, &c, 16);
  printf("a >> 16 = "); mp_print(&c, stdout); fputc('\n', stdout);

  mpl_lsh(&a, &c, 1);
  printf("a << 1 = "); mp_print(&c, stdout); fputc('\n', stdout);
  mpl_lsh(&a, &c, 5);
  printf("a << 5 = "); mp_print(&c, stdout); fputc('\n', stdout);
  mpl_lsh(&a, &c, 16);
  printf("a << 16 = "); mp_print(&c, stdout); fputc('\n', stdout);

  mpl_num_set(&a, &pco);
  printf("#1(a) = %d\n", pco);
  mpl_num_set(&b, &pco);
  printf("#1(b) = %d\n", pco);

  res = mpl_parity(&a);
  if(res == MP_EVEN)
    printf("a has even parity\n");
  else
    printf("a has odd parity\n");
  
  mp_clear(&c);
  mp_clear(&b);
  mp_clear(&a);

  return 0;
}

