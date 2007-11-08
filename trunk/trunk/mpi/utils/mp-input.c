#include <stdio.h>
#include <stdlib.h>

#include "mpi.h"

int main(int argc, char *argv[])
{
  mp_int a, b, c;

  if(argc < 3) {
    fprintf(stderr, "Usage: %s <a> <b>\n", argv[0]);
    return 1;
  }

  mp_init(&a); mp_init(&b); mp_init(&c);
  mp_read_radix(&a, (unsigned char *)argv[1], 10);
  mp_read_radix(&b, (unsigned char *)argv[2], 10);

  mp_mul(&a, &b, &c);
  
  {
    int len = mp_radix_size(&c, 10);
    char *buf = malloc(len + 1);

    mp_todecimal(&c, (unsigned char *)buf);
    printf("a + b = %s\n", buf);
    free(buf);
  }

  mp_clear(&a);
  mp_clear(&b);
  mp_clear(&c);

  return 0;
}
