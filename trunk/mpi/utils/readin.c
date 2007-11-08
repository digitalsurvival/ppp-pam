#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int main(void)
{
  char    input[128];
  mp_int  val;
  mp_err  res;

  fprintf(stderr, "Please enter a number (base 10): ");
  fgets(input, sizeof(input), stdin);
  
  mp_init(&val);
  if((res = mp_read_radix(&val, (unsigned char *)input, 10)) != MP_OKAY) {
    fprintf(stderr, "Error converting input value: %s\n",
	    mp_strerror(res));
    return 1;
  }

  {
    int out_len = mp_radix_size(&val, 10);
    unsigned char *buf = malloc(out_len);

    mp_toradix(&val, buf, 10);
    printf("You entered: %s\n", buf);
    free(buf);
  }

  mp_clear(&val);

  return 0;
}
