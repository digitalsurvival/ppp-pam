/*
  exptmod.c

  by Michael J. Fromberger <sting@linguist.dartmouth.edu>
  Copyright (C) 1999 Michael J. Fromberger, All Rights Reserved

  Command line tool to perform modular exponentiation on arbitrary
  precision integers.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.

  $Id: exptmod.c,v 1.1 2004/02/08 04:28:32 sting Exp $ 

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpi.h"

int main(int argc, char *argv[])
{
  mp_int  a, b, m;
  mp_err  res;
  unsigned char   *str;
  int     len, rval = 0;

  if(argc < 3) {
    fprintf(stderr, "Usage: %s <a> <b> <m>\n", argv[0]);
    return 1;
  }

  mp_init(&a); mp_init(&b); mp_init(&m);
  mp_read_radix(&a, (unsigned char *)argv[1], 10);
  mp_read_radix(&b, (unsigned char *)argv[2], 10);
  mp_read_radix(&m, (unsigned char *)argv[3], 10);

  if((res = mp_exptmod(&a, &b, &m, &a)) != MP_OKAY) {
    fprintf(stderr, "%s: error: %s\n", argv[0], mp_strerror(res));
    rval = 1;
  } else {
    len = mp_radix_size(&a, 10);
    str = calloc(len, sizeof(char));
    mp_toradix(&a, str, 10);

    printf("%s\n", (char *)str);

    free(str);
  }

  mp_clear(&a); mp_clear(&b); mp_clear(&m);

  return rval;
}
