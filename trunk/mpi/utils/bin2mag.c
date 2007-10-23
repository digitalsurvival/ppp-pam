/*
  bin2mag.c

  Read a binary file and convert it to an unsigned integer, expressed
  in the specified output radix.

  by Michael J. Fromberger <sting@linguist.dartmouth.edu>
  Copyright (C) 2001 Michael J. Fromberger, All Rights Reserved.

  $Id: bin2mag.c,v 1.1 2004/02/08 04:28:32 sting Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "mpi.h"

int g_base = 10;  /* output radix */

int read_file_to_int(char *fname, mp_int *v);

int main(int argc, char *argv[])
{
  mp_int    v;

  if(argc < 2) {
    fprintf(stderr, "Usage: %s <inputfile> [<output-base>]\n",
	    argv[0]);
    return 1;
  }

  if(argc > 2) {
    int  nbase = atoi(argv[2]);

    if(nbase < 2 || nbase > MAX_RADIX) {
      fprintf(stderr, "Invalid output radix '%s', using default (%d)\n", 
	      argv[2], g_base);
    } else {
      g_base = nbase;
    }
  }

  if(read_file_to_int(argv[1], &v)) {
    unsigned char *buf = malloc(mp_radix_size(&v, g_base));

    mp_toradix(&v, buf, g_base);
    printf("%s\n", (char *)buf);
    free(buf);

    return 0;
  } else {
    fprintf(stderr, "Error reading file '%s': %s\n",
	    argv[1], strerror(errno));
    return 1;
  }

}

int read_file_to_int(char *fname, mp_int *v)
{
  unsigned char *buf;
  struct stat    st;
  FILE          *ifp;
  int            nread;

  if((ifp = fopen(fname, "rb")) == NULL)
    return 0;

  if(fstat(fileno(ifp), &st) < 0) {
    fclose(ifp);
    return 0;
  }

  if((buf = malloc(st.st_size)) == NULL) {
    fclose(ifp);
    return 0;
  }

  nread = fread(buf, 1, st.st_size, ifp);
  fclose(ifp);

  mp_init(v);
  mp_read_unsigned_bin(v, buf, nread);
  free(buf);
  return 1;

}
