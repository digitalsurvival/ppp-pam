#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "mpi.h"

int main(int argc, char *argv[])
{
  int   ix;

  if(argc < 2) {
    fprintf(stderr, "Usage: magtest <value> ...\n");
    return 1;
  }

  for(ix = 1; ix < argc; ++ix) {
    mp_int  m;
    int     size, jx, brk;
    char   *buf;

    mp_init(&m);
    mp_read_radix(&m, argv[ix], 10);

    size = mp_mag_size(&m);
    printf("Value %d requires %d bytes of storage\n", ix, size);

    buf = malloc(size);
    mp_tomag(&m, buf);

    for(jx = 0, brk = 0; jx < size; ++brk, ++jx) {
      printf("%02X", buf[jx]);

      if(brk == 15) {
	fputc('\n', stdout);
	brk = -1;
      } else {
	fputc(' ', stdout);
      }
    }

    if(brk)
      fputc('\n', stdout);

    mp_read_mag(&m, buf, size);
    free(buf);
    size = mp_radix_size(&m, 10);
    buf = malloc(size);
    mp_todecimal(&m, buf);
    printf("=> %s\n", buf);

    free(buf);
    mp_clear(&m);
  }

  return 0;
}
