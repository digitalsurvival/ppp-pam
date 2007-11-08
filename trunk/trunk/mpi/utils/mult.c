/*
  mult.c

  by Michael J. Fromberger <sting@linguist.dartmouth.edu>
  Copyright (C) 1999 Michael J. Fromberger, All Rights Reserved

  Command line tool to perform multiplication on arbitrary precision
  integers.

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpi.h"

int main(int argc, char *argv[])
{
  mp_int  a, b, c;
  mp_err  res;
  unsigned char   *str;
  int     len, rval = 0;

  if(argc < 2) {
    fprintf(stderr, "Usage: %s <a> <b>\n", argv[0]);
    return 1;
  }

  mp_init(&a); mp_init(&b); mp_init(&c);
  mp_read_radix(&a, (unsigned char *)argv[1], 10);
  mp_read_radix(&b, (unsigned char *)argv[2], 10);

  if((res = mp_mul(&a, &b, &c)) != MP_OKAY) { 
    fprintf(stderr, "%s: error: %s\n", argv[0], mp_strerror(res));
    rval = 1;
  } else {
    len = mp_radix_size(&c, 10);
    str = calloc(len, sizeof(char));
    mp_todecimal(&c, str);

    printf("%s\n", (char *)str);

    free(str);
  }

  mp_clear(&a); mp_clear(&b); mp_clear(&c);

  return rval;
}
