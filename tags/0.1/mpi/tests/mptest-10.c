#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "mpi.h"

int main(int argc, char *argv[])
{
  mp_int  a[5];
  mp_err  res;
  int     i;

  res = mp_init_array(a, 5);
  printf("Result: %d (%s)\n", res, mp_strerror(res));

  mp_set(&a[0], 25);
  mp_set(&a[1], 1053);
  mp_set_int(&a[2], -15382);
  mp_mul(&a[1], &a[2], &a[3]);
  mp_sqr(&a[3], &a[4]);

  for(i = 0; i < 5; i++) {
    unsigned char *out = malloc(mp_radix_size(&a[i], 10));

    mp_todecimal(&a[i], out);
    printf("a[%d] = %s\n", i, (char *)out);
    free(out);
  }

  mp_clear_array(a, 5);

  return 0;
}
